#include <base/closure/closure_task.h>
#include <base/memory/lazy_instance.h>

#include "coroutine_runner.h"
#include "glog/logging.h"

namespace {
static const char* kCoSleepWarning =
  "blocking co_sleep is hit, consider move you task into coro";

}

namespace base {

static std::once_flag backgroup_once;

static thread_local CoroRunner* tls_runner = NULL;

static const int kMaxReuseCoroutineNumbersPerThread = 500;

ConcurrentTaskQueue CoroRunner::stealing_queue;

//IMPORTANT NOTE: NO HEAP MEMORY HERE!!!
#ifdef USE_LIBACO_CORO_IMPL
void CoroRunner::CoroutineEntry() {
  Coroutine* coroutine = static_cast<Coroutine*>(aco_get_arg());
#else
void CoroRunner::CoroutineEntry(void *coro) {
  Coroutine* coroutine = static_cast<Coroutine*>(coro);
#endif

  CHECK(coroutine);

  TaskBasePtr task;
  do {
#if 0
    ST next = tls_runner->GetTask(task);
    if (task) {
      task->Run(); task.reset();
    }
    if (res.ST == kParking) {
      tls_runner->YieldInternal();
    }
  } while(res.ST != kExit);
#endif
    DCHECK(coroutine->IsRunning());
    if (tls_runner->GetTask(task)) {
      task->Run(); task.reset();
    };
  }while(tls_runner->ContinueRun());

  task.reset();
  coroutine->SetCoroState(CoroState::kDone);
  tls_runner->to_be_delete_.push_back(coroutine);

#ifdef USE_LIBACO_CORO_IMPL
  // libaco aco_exti will swapout current and jump back pthread flow
  tls_runner->current_ = tls_runner->main_coro_;
  coroutine->Exit();
#else
  /* libcoro has the same flow for exit the finished coroutine*/
  tls_runner->YieldInternal();
#endif
}

CoroRunner& CoroRunner::instance() {
  static thread_local CoroRunner runner;
  return runner;
}

//static
MessageLoop* CoroRunner::backgroup() {
  static LazyInstance<MessageLoop> loop = LAZY_INSTANCE_INIT;
  std::call_once(backgroup_once, [&]() {
    loop->SetLoopName("coro_background"); loop->Start();
    //initialized a runner binded to this loop
    loop->PostTask(FROM_HERE, &CoroRunner::instance);
  });
  return loop.ptr();
}

//static
void CoroRunner::RegisteAsCoroWorker(MessageLoop* l) {
  l->PostTask(FROM_HERE, &CoroRunner::instance);
}

//static
bool CoroRunner::Yieldable() {
  return tls_runner ? (!tls_runner->IsMain()) : false;
}

//static
void CoroRunner::Yield() {
  CHECK(Yieldable());
  tls_runner->YieldInternal();
}

//static
void CoroRunner::Sleep(uint64_t ms) {
  if (!Yieldable()) {
    LOG(INFO) << kCoSleepWarning;
    usleep(ms * 1000);
    return;
  }

  auto loop = tls_runner->bind_loop_;
  if (!loop->PostDelayTask(NewClosure(MakeResumer()), ms)) {
    usleep(ms * 1000);
    return;
  }
  tls_runner->YieldInternal();
}

//static
LtClosure CoroRunner::MakeResumer() {
  if (!tls_runner) {
    return nullptr;
  }
  auto weak_coro = tls_runner->current_->AsWeakPtr();
  uint64_t resume_id = tls_runner->current_->ResumeId();
  return std::bind(&CoroRunner::Resume, tls_runner, weak_coro, resume_id);
}

//static
bool CoroRunner::ScheduleTask(TaskBasePtr&& task) {
  return stealing_queue.enqueue(std::move(task));
}

CoroRunner::CoroRunner()
  : gc_task_scheduled_(false),
    bind_loop_(MessageLoop::Current()),
    max_parking_count_(kMaxReuseCoroutineNumbersPerThread) {

  CHECK(bind_loop_);

  tls_runner = this;
  main_ = Coroutine::CreateMain();
  current_ = main_coro_ = main_.get();

  bind_loop_->InstallPersistRunner(this);
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " initialized";
}

CoroRunner::~CoroRunner() {
  FreeOutdatedCoro();
  for (auto coro : stash_list_) {
    coro->ReleaseSelfHolder();
  }
  stash_list_.clear();

  current_ = nullptr;
  main_coro_ = nullptr;
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " gone";
}

bool CoroRunner::HitDeadLine() const {
  return base::time_us() - last_run_timestamp_ > 5000;
}

bool CoroRunner::GetTask(TaskBasePtr& task) {
  if (!coro_tasks_.empty()) {
    task = std::move(coro_tasks_.front());
    coro_tasks_.pop_front();
    return true;
  }

  // steal from global task queue
  return stealing_queue.try_dequeue(task);
}

bool CoroRunner::HasMoreTask() const {
  return coro_tasks_.size() || stealing_queue.size_approx();
}

void CoroRunner::Sched() {
  // get a corotuien and start to run
  // last_run_timestamp_ = base::time_us();

  while (HasMoreTask()) {

    //P(CoroRunner) take a M(Corotine) do work(TaskBasePtr)
    Coroutine* coro = RetrieveCoroutine();
    DCHECK(coro);

    SwapCurrentAndTransferTo(coro);
  }
  FreeOutdatedCoro();
}

/* 如果本身是在一个子coro里面,
 * 需要在重新将resume调度到MainCoro内
 * 如果本身是MainCoro，则直接进行切换.
 * 如果不是在调度的线程里,则调度到目标Loop去Resume*/
void CoroRunner::Resume(std::weak_ptr<Coroutine>& weak, uint64_t id) {
  if (bind_loop_->IsInLoopThread() && IsMain()) {
    return DoResume(weak, id);
  }
  auto f = std::bind(&CoroRunner::DoResume, this, weak, id);
  CHECK(bind_loop_->PostTask(NewClosure(f)));
}

void CoroRunner::DoResume(WeakCoroutine& weak, uint64_t id) {
  DCHECK(IsMain());

  Coroutine* coroutine = nullptr;
  {
    auto coro = weak.lock();
    if (!coro) {return;}

    coroutine = coro.get();
  }
  if (!coroutine->CanResume(id)) {
    return;
  }
  return SwapCurrentAndTransferTo(coroutine);
}

void CoroRunner::SwapCurrentAndTransferTo(Coroutine *next) {
  if (next == current_) {
    LOG(INFO) << "coro: next_coro == current, do nothing";
    return;
  }
  Coroutine* current = current_;

  current_ = next;

  VLOG(GLOG_VTRACE) << __func__
    << RunnerInfo() << " switch to next:" << current_;
  current->TransferTo(next);
}

bool CoroRunner::ContinueRun() {
  if (HasMoreTask()) {
    return true;
  }

  // enough parking coroutine, end this coroutine
  if (stash_list_.size() > max_parking_count_) {
    return false;
  }

  // parking coroutine
  stash_list_.push_back(current_);
  SwapCurrentAndTransferTo(main_coro_);
  return true;
}

Coroutine* CoroRunner::RetrieveCoroutine() {
  Coroutine* coro = nullptr;
  while(stash_list_.size() && !coro) {
    coro = stash_list_.front();
    stash_list_.pop_front();
  }

  if (coro) {
    return coro;
  }

  auto coro_ptr =
#ifdef USE_LIBACO_CORO_IMPL
    Coroutine::Create(&CoroRunner::CoroutineEntry, main_coro_);
#else
    Coroutine::Create(&CoroRunner::CoroutineEntry);
#endif
  coro_ptr->SelfHolder(coro_ptr);

  return coro_ptr.get();
}

void CoroRunner::FreeOutdatedCoro() {
  for (auto& coro : to_be_delete_) {
    coro->ReleaseSelfHolder();
  }
  to_be_delete_.clear();
  gc_task_scheduled_ = false;
}


std::string CoroRunner::RunnerInfo() const {
  std::ostringstream oss;
  oss << "[ current:" << current_
      << ", wait_id:" << current_->ResumeId()
      << ", status:" << current_->StateToString()
      << ", is_main:" << (IsMain() ? "true" : "false") << "] ";
  return oss.str();
}

}// end base
