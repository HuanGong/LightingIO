#ifndef BASE_MESSAGE_LOOP_DELEGATE_H_H
#define BASE_MESSAGE_LOOP_DELEGATE_H_H

namespace base {

class MessageLoop2;

class LoopDelegate {
public:
  virtual ~LoopDelegate();
  virtual void BeforeMessageLoopRun(base::MessageLoop2* l) = 0;
  virtual void AfterMessageLoopRun(base::MessageLoop2* l) = 0;
}

}
#endif
