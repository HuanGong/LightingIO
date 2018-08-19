#ifndef _NET_PROTOCOL_REDIS_RESPONSE_H_
#define _NET_PROTOCOL_REDIS_RESPONSE_H_

#include <vector>
#include <memory>
#include "base/base_micro.h"
#include "protocol/proto_message.h"
#include "thirdparty/resp/resp/all.hpp"

namespace net {

class RespService;
class RedisResponse;
typedef std::shared_ptr<RedisResponse> RefRedisResponse;

class RedisResponse : public ProtocolMessage {
public:
  RedisResponse();
  ~RedisResponse();

  size_t Count() const {return results_.size();}
  const resp::unique_value& ResultAtIndex(size_t idx) const {return results_[idx];}
private:
  friend class RespService;
  void AddResult(resp::result& result);

  std::vector<resp::unique_value> results_;
};

} //end namesapce
#endif
