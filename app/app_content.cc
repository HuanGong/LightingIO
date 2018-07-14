
#include "app_content.h"
#include "glog/logging.h"
#include <base/closure/closure_task.h>
#include <base/message_loop/message_loop.h>

namespace content {

App::App() {
  content_loop_.SetLoopName("MainLoop");
  content_loop_.Start();
}

App::~App() {
}

base::MessageLoop* App::MainLoop() {
  return &content_loop_;
}

void App::RunApplication() {
  auto functor = std::bind(&App::ContentMain, this);
  content_loop_.PostTask(base::NewClosure(functor));

  content_loop_.WaitLoopEnd();
}

void App::ContentMain() {
  LOG(INFO) << " Applictaion Content Main Run...";
}

}
