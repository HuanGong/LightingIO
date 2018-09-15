#ifndef LT_COMPONENT_LRU_CACHE_H
#define LT_COMPONENT_LRU_CACHE_H

#include <list>
#include <utility>
#include <unordered_map>

namespace component {

  template<typename K, typename V>
    class LRUCache {
      public:
        LRUCache(size_t campacity) : campacity_(campacity) {};
      private:
        const size_t campacity_;
        typedef typename std::pair<K, V> holder_t;
        typedef typename std::list<holder_t>::iterator list_iterator_t;

        std::list<holder_t> cache_items_list_;
        std::unordered_map<K, list_iterator_t> cache_items_map_;

      public:
        void Add(const K& k, V&& v) {
          auto it = cache_items_map_.find(k);
          if (it == cache_items_map_.end()) {
            if (size() >= campacity_) {
              auto last = cache_items_list_.end();
              --last;
#if defined(__cpp_lib_node_extract)
              auto nh = cache_items_map_.extract(last->first);
              nh.key() = k;
              cache_items_map_.insert(std::move(nh));

              last->first = k;
              last->second = std::forward<V>(v);
              cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, last);
              return;
#else
              cache_items_map_.erase(last->first);
              cache_items_list_.pop_back();
#endif
            }
            cache_items_list_.emplace_front(k, std::forward<V>(v));
            cache_items_map_[k] = cache_items_list_.begin();
          } else {
            it->second->second = std::forward<V>(v);
            cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
          }
        }

        bool Get(const K & k, V* v) {
          auto it = cache_items_map_.find(k);
          if (it == cache_items_map_.cend()) {
            return false;
          }
          //const_cast<LRUCache<K,V> *>(this)->cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
          cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
          *v = it->second->second;
          return true;
        }

        bool Exists(const K& k) const {
          return cache_items_map_.find(k) != cache_items_map_.cend();
        }

        inline size_t size() const {
          return cache_items_map_.size();
        }
    };
} //end namespace component
#endif
