#include "codec_service.h"
#include "codec_message.h"

#include <unistd.h>
#include <sys/mman.h>

#include "glog/logging.h"
#include <net_io/tcp_channel.h>
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

CodecService::CodecService(base::MessageLoop* loop)
  : binded_loop_(loop) {
}

CodecService::~CodecService() {
  VLOG(GLOG_VTRACE) << __func__ << " this@" << this << " gone";
};

void CodecService::SetDelegate(CodecService::Delegate* d) {
	delegate_ = d;
}

bool CodecService::IsConnected() const {
	return channel_ ? channel_->IsConnected() : false;
}

bool CodecService::BindToSocket(int fd,
                                const IPEndPoint& local,
                                const IPEndPoint& peer) {
  channel_ = TcpChannel::Create(fd, local, peer, Pump());
	channel_->SetReciever(this);
  return true;
}

void CodecService::StartProtocolService() {
  CHECK(binded_loop_->IsInLoopThread());
  channel_->StartChannel();
}

void CodecService::CloseService(bool block_callback) {
	DCHECK(binded_loop_->IsInLoopThread());

	BeforeCloseService();

  if (!block_callback) {
	  return channel_->ShutdownChannel(false);
  }
  return channel_->ShutdownWithoutNotify();
}

void CodecService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
}

void CodecService::OnChannelClosed(const SocketChannel* channel) {
	CHECK(channel == channel_.get());

	VLOG(GLOG_VTRACE) << __FUNCTION__ << channel_->ChannelInfo() << " closed";
	RefCodecService guard = shared_from_this();

	AfterChannelClosed();

  delegate_->OnProtocolServiceGone(guard);
}

void CodecService::OnChannelReady(const SocketChannel*) {
  RefCodecService guard = shared_from_this();
  delegate_->OnProtocolServiceReady(guard);
}

}}// end namespace
