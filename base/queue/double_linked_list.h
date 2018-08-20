#ifndef _BASE_DOUBLELINKED_LIST_H_
#define _BASE_DOUBLELINKED_LIST_H_

#include <cstdio>
#include <memory>
#include <inttypes.h>
#include <functional>
#include <base/base_micro.h>

namespace base {

namespace __detail {

struct Holder {
};

}
/*
template<typename T>
struct DoubleLinkedTraits {
  inline static T* pre(T* t) {return t->pre_;}
  inline static T* next(T* t) {return t->next_;}
  inline static void set_pre(T* t, T* pre) {t->pre_ = pre;}
  inline static void set_next(T* t, T* next) {t->next_ = next;}
  inline static DoubleLinkedItemHolder* holder(T* t) {return t->holder_;}
  inline static void set_holder(T* t, DoubleLinkedItemHolder* h) {t->holder_ = h;};
};
*/

template<class T>
struct EnableDoubleLinked {
  EnableDoubleLinked<T>* pre_ = NULL;
  EnableDoubleLinked<T>* next_ = NULL;
  __detail::Holder* holder_ = NULL;
};

template<class T>
class DoubleLinkedList : public __detail::Holder {
public:
  DoubleLinkedList() : size_(0) {
    tail_ = new EnableDoubleLinked<T>();
    head_ = new EnableDoubleLinked<T>();

    tail_->pre_ = head_;
    head_->next_ = tail_;
    head_->pre_ = nullptr;
    tail_->next_ = nullptr;
  }

  ~DoubleLinkedList() {

    EnableDoubleLinked<T>* node = head_->next_;
    for (; node != tail_; node = node->next_) {
      Remove((T*)node);
    }
    size_ = 0;
    delete tail_;
    delete head_;
  }

  inline uint64_t Size() const {return size_;}

  inline bool Attatched(T* node) {
    return node->holder_ == (__detail::Holder*)this;
  }

  bool PushBack(T* node) {
    if (!node || Attatched(node)) {
      return false;
    }
    tail_->pre_->next_ = node;
    node->pre_ = tail_->pre_;
    tail_->pre_ = node;
    node->next_ = tail_;
    node->holder_ = this;
    size_++;
    return true;
  }

  T* Back() {
    return size_ > 0 ? (T*)tail_->pre_ : nullptr;
  }

  T* Front() {
    return size_ > 0 ? (T*)head_->next_ : nullptr;
  }

  bool Remove(T* node) {
    if (size_ <= 0 || !Attatched(node)) {
      return false;
    }
    node->pre_->next_ = node->next_;
    node->next_->pre_ = node->pre_;
    node->pre_ = nullptr;
    node->next_ = nullptr;
    node->holder_ = nullptr;
    size_--;
    return true;
  }

  bool PushFront(T* node) {
    if (!node || Attatched(node)) {
      return false;
    }
    head_->next_->pre_ = node;
    node->next_ = head_->next_;
    head_->next_ = node;
    node->pre_ = head_;
    node->holder_ = this;
    size_++;
    return true;
  }

  void Foreach(std::function<void(const T*)> func) {
    const DoubleLinkedList<T>* node = head_->next_;
    for (; node != tail_; node = node->next_) {
      func((T*)node);
    }
  }

private:
  EnableDoubleLinked<T>* head_;
  EnableDoubleLinked<T>* tail_;
  uint64_t size_;
  DISALLOW_COPY_AND_ASSIGN(DoubleLinkedList);
};


} // namesapce base end
#endif
