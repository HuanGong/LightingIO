#ifndef NET_SERVER_H_H
#define NET_SERVER_H_H

#include <string>
#include <memory>
#include <thread>

#include "base/message_loop/message_loop.h"
#include "io_service.h"

namespace net {

template<class T> using UniquePtrList = std::vector<std::unique_ptr<T>>;
template<class T> using SharedPtrList = std::vector<std::shared_ptr<T>>;

using WorkerContainer = SharedPtrList<base::MessageLoop>;

class Server {
public:
  Server(std::string name)
    : name_(name) {
  }

  virtual ~Server() {};

  virtual void Initialize() = 0;
  virtual void Run() = 0;
  virtual void Stop() = 0;

  std::string ServerName() {
    return name_;
  }
private:
  std::string name_;
};

class SrvDelegate {
public:
  virtual ~SrvDelegate() {}

  virtual void OnIoServiceInit(IoService* io_service) {};
  virtual void OnIoServiceRun(IoService* io_service) {};

  // assign worker loop for server, return numbers workers
  // @params in container: a container take thoes worker;
  virtual uint32_t CreateWorkersForServer(WorkerContainer& container);

  //provide address and ports for listen
  // return vector<string> like: 0.0.0.0:6666
  virtual std::vector<std::string> HttpListenAddress();

  // see HttpListenAddress
  virtual std::vector<std::string> TcpListenAddress();

  virtual void OnWorkerStarted(base::MessageLoop* worker) {};
};

} //end namespace net
#endif
