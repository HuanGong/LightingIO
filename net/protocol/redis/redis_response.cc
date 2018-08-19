#include "redis_response.h"

namespace net {

RedisResponse::RedisResponse() :
  ProtocolMessage("redis") {

  ProtocolMessage::SetMessageType(kResponse);
}

RedisResponse::~RedisResponse() {
}

void RedisResponse::AddResult(resp::result& result) {
  results_.push_back(result.value()); 
}

}//end namespace
