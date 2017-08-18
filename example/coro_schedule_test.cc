#include "libcoro/coro.h"
#include "message_loop/message_loop.h"

struct coro_stack stack;
coro_context mainctx;

std::vector<coro_context> coro_tasks;

void coro_task(void* arg) {
  LOG(INFO) << "coro task run on thread" << std::this_thread::get_id();
  coro_transfer(&coro_tasks[0], &mainctx);
}

int main(int arvc, char **argv) {

  base::MessageLoop loop("loop");
  loop.Start();
  LOG(INFO) << "main thread" << std::this_thread::get_id();

  loop.PostTask(base::NewClosure([&](){
    //atatch main coro to current thread
    coro_stack_alloc(&stack, 0);
    coro_create(&mainctx, NULL, NULL, NULL, 0);

    coro_context task;
    coro_create(&task, coro_task, NULL, stack.sptr, stack.ssze);
    coro_tasks.push_back(task);
  }));

  loop.PostTask(base::NewClosure([&](){
    LOG(INFO) << "going handle http request";
    usleep(2000); //sleep 1ms

    //response = httpclient_->Get("0.0.0.0/abc");

  }));

    coro_transfer(&mainctx, &coro_tasks[0]);
    LOG(INFO) << "after run coro_task, schedule it again, core?";
    //coro_transfer(&mainctx, &task);
    //LOG(INFO) << "run coro_task again, it work and run again";

  while(1) {
    sleep(2);
  }
}
