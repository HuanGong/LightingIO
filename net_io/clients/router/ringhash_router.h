#ifndef _LT_NET_RINGHASH_ROUTER_H_H
#define _LT_NET_RINGHASH_ROUTER_H_H

#include <zlib.h> //crc32

#include "client_router.h"
#include "murmurhash/MurmurHash3.h"
#include "net_io/base/load_balance/consistent_hash_map.h"

namespace lt {
namespace net {

class RingHashRouter : public ClientRouter {
public:
  struct ClientNode {
    ClientNode(RefClient client, uint32_t id);
    RefClient client;
    const uint32_t vnode_id;
    const std::string hash_key;
  };

  struct NodeHasher {
    typedef uint32_t result_type;
    result_type operator()(const ClientNode& node);
  };

  typedef lb::ConsistentHashMap<ClientNode, NodeHasher> NodeContainer;
public:
    RingHashRouter(uint32_t vnode_count);
    RingHashRouter() : RingHashRouter(50) {};
    ~RingHashRouter() {};

    void AddClient(RefClient&& client) override;
    RefClient GetNextClient(const std::string& key,
                            CodecMessage* request = NULL) override;
  private:
    NodeContainer clients_;
    const uint32_t vnode_count_ = 50;
};

}} //lt::net
#endif
