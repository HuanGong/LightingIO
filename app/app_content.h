#ifndef APP_CONTENT_H_H
#define APP_CONTENT_H_H

#include <base/event_loop/msg_event_loop.h>

namespace content {

class App {
public:
  App();
  virtual ~App();

  void RunApplication();
  base::MessageLoop2* MainLoop();
protected:
  virtual void ContentMain();

  virtual void BeforeApplicationRun() = 0;
  virtual void AfterApplicationRun() = 0;

private:
  base::MessageLoop2 content_loop_;
};

} //namespace content
#endif
