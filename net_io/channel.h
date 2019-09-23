//
// Created by gh on 18-12-5.
//

#ifndef LIGHTINGIO_NET_CHANNEL_H
#define LIGHTINGIO_NET_CHANNEL_H

#include "address.h"
#include "io_buffer.h"
#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"

// socket chennel interface and base class
namespace lt {
namespace net {


class SocketChannel {
public:
  enum class Status {
    CONNECTING,
    CONNECTED,
	  CLOSING,
    CLOSED
  };
  typedef struct Reciever {
    virtual void OnChannelReady(const SocketChannel*) {};
    virtual void OnStatusChanged(const SocketChannel*) {};
    virtual void OnDataFinishSend(const SocketChannel*) {};
    virtual void OnChannelClosed(const SocketChannel*) = 0;
    virtual void OnDataReceived(const SocketChannel*, IOBuffer *) = 0;
  }Reciever;
public:
  void Start();
  void SetReciever(SocketChannel::Reciever* consumer);

  base::MessageLoop* IOLoop() const {return io_loop_;}
  bool InIOLoop() const {return io_loop_->IsInLoopThread();};
  bool IsConnected() const {return status_ == Status::CONNECTED;};


  std::string ChannelInfo() const;
  const std::string StatusAsString() const;
  const std::string& ChannelName() const {return name_;}
protected:
  SocketChannel(int socket_fd,
                const SocketAddress& loc,
                const SocketAddress& peer,
                base::MessageLoop* loop);
  virtual ~SocketChannel() {}

  void OnChannelReady();
  void SetChannelStatus(Status st);

  /* channel relative things should happened on io_loop*/
  base::MessageLoop* io_loop_;

  bool schedule_shutdown_ = false;
  Status status_ = Status::CLOSED;

  base::RefFdEvent fd_event_;

  SocketAddress local_addr_;
  SocketAddress peer_addr_;

  std::string name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  SocketChannel::Reciever* reciever_ = NULL;
  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};

}} //lt::net
#endif //LIGHTINGIO_NET_CHANNEL_H
