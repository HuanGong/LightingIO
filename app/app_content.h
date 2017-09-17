#ifndef APP_CONTENT_H_H
#define APP_CONTENT_H_H

namespace content {




template<class ContentCtx>
class App {
public:
  App();
  virtual ~App();

  AppDelegate* Delegate();

  ContentCtx* ApplicationCtx();
  void InstallCtx(ContentCtx* ctx);


priavte:
  AppDelegate* delegate_;
  ContentCtx* content_ctx_;
};

} //namespace content
#endif
