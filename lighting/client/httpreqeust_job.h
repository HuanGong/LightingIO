#ifndef HTTP_URL_REQUEST_JOB_H_
#define HTTP_URL_REQUEST_JOB_H_

#include "url_request_job.h"

namespace net {

class HttpUrlRequestJob : public UrlRequestJob {
public:
  HttpUrlRequestJob();
  ~HttpUrlRequestJob();

  void SetClientChannel(ClientChannel* connection) {
    connection_ = connection;
  }
  void Start() override;
private:
  ClientChannel* connection_;
};

} //end name space
#endif
