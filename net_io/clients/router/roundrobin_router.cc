
#include "roundrobin_router.h"

namespace lt {
namespace net {

void RoundRobinRouter::StopAllClients() {
  for (auto& client : clients_) {
    client->Finalize();
  }
}

void RoundRobinRouter::AddClient(ClientPtr&& client) {
  clients_.push_back(std::move(client));
}

Client* RoundRobinRouter::GetNextClient(const std::string& key,
                                        ProtocolMessage* request) {
  uint32_t idx = round_index_++ % clients_.size();
  return clients_[idx].get();
}

}}//end lt::net
