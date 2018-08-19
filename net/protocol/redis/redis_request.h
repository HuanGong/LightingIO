#ifndef _NET_PROTOCOL_REDIS_MESSAGE_H_
#define _NET_PROTOCOL_REDIS_MESSAGE_H_

#include <vector>
#include "base/base_micro.h"
#include "redis_response.h"
#include "protocol/proto_message.h"
#include "thirdparty/resp/resp/all.hpp"

namespace net {

class RespService;
class RedisRequest;
typedef std::shared_ptr<RedisRequest> RefRedisRequest;

class RedisRequest : public ProtocolMessage {
public:
  typedef RedisResponse ResponseType;

  RedisRequest();
  ~RedisRequest();

  void Get(const std::string& key);
  void MGet(const std::vector<std::string>& keys);

  void Set(const std::string& key, const std::string& value);
  void MSet(const std::vector<std::pair<std::string, std::string>> kvs);
  void SetWithExpire(const std::string& key, const std::string& value, uint32_t second);

  void Exists(const std::string& key);
  void Delete(const std::string& key);

  void Incr(const std::string& key);
  void IncrBy(const std::string& key, uint64_t by);
  void IncrByFloat(const std::string& key, float by);
  void Decr(const std::string& key);
  void DecrBy(const std::string& key, uint64_t by);

  void Select(const std::string& db);
  void Auth(const std::string& password);

  void TTL(const std::string& key);
  void Persist(const std::string& key);
  void Expire(const std::string& key, uint64_t second);
/*
  void HDel(const std::string& hash_container, const std::string& field);
  void HSet(const std::string& hash_container, const std::string& field, const std::string& value);
  void HGet(const std::string& hash_container, const std::string& field);
  void HGetAll(const std::string& hash_container);
  void HExisit(const std::string& hash_container, const std::string& field);
  void HIncrBy(const std::string& hash_container, const std::string& field);
  void HIncrByFloat(const std::string& hash_container, const std::string& field);
  void HKeys(const std::string& hash_container);
  void HLen(const std::string& hash_container);
  void HMGet(const std::string& hash_container, std::vector<std::string> fields);
  void HMSet(const std::string& hash_container, std::vector<std::pair<std::string, std::string>> kvs);
  void HSetNotExist(const std::string& hash_container, const std::string& field, const std::string& value);
  //HScan
  //

  void SAdd(const std::string& set, const std::vector<std::string>& fields);
  void SMembers(const std::string& set, const std::string& field);
  void SDiff(const std::string& set1, const std::string& set2);
  void SIsMember(const std::string& set, const std::string& field);
*/

  inline uint32_t CmdCount() const {return cmd_counter_;}
private:
  friend class RespService;

  std::string body_;
  uint32_t cmd_counter_ = 0;
  resp::encoder<resp::buffer> encoder_;
};

} //end namesapce
#endif
