#ifndef URL_REQUEST_JOB_H_
#define URL_REQUEST_JOB_H_

namespace net {

class UrlRequestJob {
public:
  UrlRequestJob();
  virtual ~UrlRequestJob();

  virtual Start() = 0;

private:

};

}//end net
#endif
