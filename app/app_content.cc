
#include "app_content.h"

namespace content {

App::App(AppDelegate* delegate);
App::~App();

AppDelegate* App::Delegate() {
  return delegate_;
}

ContentCtx* App::ApplicationCtx() {
  return content_ctx_;
}

void App::InstallCtx(ContentCtx* ctx) {
  content_ctx_ = ctx;
}


}
