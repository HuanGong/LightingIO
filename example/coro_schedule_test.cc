#include "libcoro/coro.h"
#include "message_loop/message_loop.h"

#include "coroutine/coroutine.h"

int main(int arvc, char **argv) {

  base::Coroutine coro(0, true);

  base::MessageLoop loop("loop");
  loop.Start();
  LOG(INFO) << "main thread" << std::this_thread::get_id();

  loop.PostTask(base::NewClosure([&](){

    {
      base::Coroutine subcoro(0);
      subcoro.SetCaller(&coro);

      LOG(INFO) << "schedule subcoro";
      coro.Transfer(&subcoro);
    }

    LOG(INFO) << "io loop task finished";
  }));

  /*
    sleep(5);
    loop.PostTask(base::NewClosure([&](){
      LOG(INFO) << "io loop live, schedule remain coro_task";
      coro_transfer(&mainctx, &coro_task);
      LOG(INFO) << "return to mainctx";
    }));
  */
  while(1) {
    sleep(2);
  }
}
