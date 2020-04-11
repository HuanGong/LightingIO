#ifndef NET_PROTOCOL_SERVICE_FACTORY_H
#define NET_PROTOCOL_SERVICE_FACTORY_H

#include "codec_service.h"

#include <memory>
#include <functional>
#include <unordered_map>
#include <net_io/net_callback.h>
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

typedef std::function<RefCodecService (base::MessageLoop* loop)> ProtoserviceCreator;

class CodecFactory {
public:
  static RefCodecService NewServerService(const std::string& proto,
                                          base::MessageLoop* loop);
  static RefCodecService NewClientService(const std::string& proto,
                                          base::MessageLoop* loop);

  static bool HasCreator(const std::string&);

  // not thread safe,
  // this can cover the default protoservice or add new protocol support
  static void RegisterCreator(const std::string, ProtoserviceCreator);
public:
  CodecFactory();
private:
  void InitInnerDefault();

  RefCodecService CreateProtocolService(const std::string&,
                                        base::MessageLoop*,
                                        bool server_service);

  std::unordered_map<std::string, ProtoserviceCreator> creators_;
};

}}
#endif
