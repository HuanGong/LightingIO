#include "redis_request.h"

namespace net {

RedisRequest::RedisRequest() :
  ProtocolMessage("redis") {
  ProtocolMessage::SetMessageType(kRequest);
}

RedisRequest::~RedisRequest() {
}

void RedisRequest::Get(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("GET", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::MGet(const std::vector<std::string>& keys) {

  std::vector<resp::buffer> buffers;
  encoder_.begin(buffers);
  auto cmder = encoder_.cmd("MGET");
  for (auto& key : keys) {
    cmder.arg(key);
  }
  cmder.end();
  encoder_.end();

  for (auto& buffer :  buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Set(const std::string& key, const std::string& value) {
  std::vector<resp::buffer> buffers = encoder_.encode("SET", key, value);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::SetWithExpire(const std::string& key, const std::string& value, uint32_t second) {

  std::vector<resp::buffer> buffers = encoder_.encode("SET", key, value, "EX", std::to_string(second));

  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  //LOG(INFO) << __FUNCTION__ << " body:" << body_;
  cmd_counter_++;
}

void RedisRequest::MSet(const std::vector<std::pair<std::string, std::string>> kvs) {

  std::vector<resp::buffer> buffers;
  encoder_.begin(buffers);
  auto cmder = encoder_.cmd("MSET");
  for (auto& kv : kvs) {
    cmder.arg(kv.first).arg(kv.second);
  }
  cmder.end();
  encoder_.end();

  for (auto& buffer :  buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Exists(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("EXISTS", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Delete(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("DEL", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Incr(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("INCR", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::IncrBy(const std::string& key, uint64_t by) {
  std::vector<resp::buffer> buffers = encoder_.encode("INCRBY", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::IncrByFloat(const std::string& key, float by) {
  std::vector<resp::buffer> buffers = encoder_.encode("INCRBYFLOAT", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Decr(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("DECR", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::DecrBy(const std::string& key, uint64_t by) {
  std::vector<resp::buffer> buffers = encoder_.encode("DECRBY", key, std::to_string(by));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Select(const std::string& db) {
  std::vector<resp::buffer> buffers = encoder_.encode("SELECT", db);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Auth(const std::string& password) {
  std::vector<resp::buffer> buffers = encoder_.encode("AUTH", password);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::TTL(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("TTL", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Persist(const std::string& key) {
  std::vector<resp::buffer> buffers = encoder_.encode("PERSIST", key);
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

void RedisRequest::Expire(const std::string& key, uint64_t second) {
  std::vector<resp::buffer> buffers = encoder_.encode("EXPIRE", key, std::to_string(second));
  for (auto& buffer : buffers) {
    body_.append(buffer.data(), buffer.size());
  }
  cmd_counter_++;
}

} //end namespace
