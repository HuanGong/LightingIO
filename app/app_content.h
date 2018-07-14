#ifndef APP_CONTENT_H_H
#define APP_CONTENT_H_H

#include <base/message_loop/message_loop.h>

namespace content {

class App {
public:
  App();
  virtual ~App();

  void RunApplication();
  base::MessageLoop* MainLoop();
protected:
  virtual void ContentMain();

  virtual void BeforeApplicationRun() = 0;
  virtual void AfterApplicationRun() = 0;

private:
  base::MessageLoop content_loop_;
};

} //namespace content
#endif
