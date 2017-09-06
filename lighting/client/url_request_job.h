#ifndef URL_REQUEST_JOB_H_
#define URL_REQUEST_JOB_H_
/*
 * UrlReqeust reqeust;
 * auto httprequestjob = UrlReqeustJobFactory::Create(UrlReqeust* request);
 * httprequestjob->SendRecieve();
 *
 * */
namespace net {
//take care of thing and life
class UrlRequestJob {
public:
  UrlRequestJob();
  virtual ~UrlRequestJob();

  virtual bool SendRecieve() = 0;
};

}//end net
#endif
