#ifndef _COMPONENT_DOUBLELINKEDLIST_H_
#define _COMPONENT_DOUBLELINKEDLIST_H_

#include <cstdio>
#include <inttypes.h>

namespace base {

struct DoubleLinkedItemHolder {
};

template<typename T>
struct DoubleLinkedTraits {
  inline static T* pre(T* t) {return t->pre_;}
  inline static T* next(T* t) {return t->next_;}
  inline static void set_pre(T* t, T* pre) {t->pre_ = pre;}
  inline static void set_next(T* t, T* next) {t->next_ = next;}
  inline static DoubleLinkedItemHolder* holder(T* t) {return t->holder_;}
  inline static void set_holder(T* t, DoubleLinkedItemHolder* h) {t->holder_ = h;};
};

template<class T>
struct EnableDoubleLinked {
  T* pre_ = NULL;
  T* next_ = NULL;
  DoubleLinkedItemHolder* holder_ = NULL;
};

template<class T>
class DLinkedList : public DoubleLinkedItemHolder {
public:
  typedef DoubleLinkedTraits<T> Traits;
  DLinkedList() :
    head_(NULL),
    tail_(NULL),
    size_(0) {
  }

  T* front() {
    return head_;
  }

  void pop_front() {
    if (!head_) {return;}

    T* removed = head_;
    head_ = Traits::next(head_);

    Traits::set_pre(removed, NULL);
    Traits::set_next(removed, NULL);
    if (head_) {
      Traits::set_pre(head_, NULL);
    } else {
      tail_ = NULL;
    }
    Traits::set_holder(removed, NULL);
    --size_;
  }

  bool push_front(T* node) {
    if (!node || Attatched(node)) {
      return false;
    }

    Traits::set_pre(node, NULL);
    if (head_ == NULL) {
      head_ = tail_ = node;
      Traits::set_next(node, NULL);
    } else {
      T* second = head_;
      Traits::set_pre(second, node);
      Traits::set_next(node, second);
      head_ = node;
    }
    Traits::set_holder(this);
    ++size_;
    return true;
  }

  T* back() {
    return tail_;
  }

  void pop_back() {
    if (!tail_) {return;}

    T* removed = tail_;
    tail_ = Traits::pre(removed);

    Traits::set_pre(removed, NULL);
    Traits::set_next(removed, NULL);

    if (tail_) {
      Traits::set_next(tail_, NULL);
    } else {
      head_ = NULL;
    }
    Traits::set_holder(removed, NULL);
    --size_;
  }

  bool push_back(T* node) {
    if (!node || Attatched(node)) {
      return false;
    }

    Traits::set_next(node, NULL);
    if (tail_ == NULL) {
      head_ = tail_ = node;
      Traits::set_pre(node, NULL);
    } else {
      Traits::set_next(tail_, node);
      Traits::set_pre(node, tail_);
      tail_ = node;
    }
    Traits::set_holder(node, this);
    ++size_;
    return true;
  }

  bool remove(T* node) {
    if (!node || !Attatched(node)) {
      return false;
    }

    T* pre = Traits::pre(node);
    T* next = Traits::next(node);
    if (next) {
      Traits::set_pre(next, pre);
    } else {
      tail_ = pre;
    }
    if (pre) {
      Traits::set_next(pre, next);
    } else {
      head_ = next;
    }

    Traits::set_pre(node, NULL);
    Traits::set_next(node, NULL);
    Traits::set_holder(node, NULL);
    --size_;
    return true;
  }

  inline uint64_t size() const {return size_;}
  inline bool Attatched(T* node) {
    return Traits::holder(node) == (DoubleLinkedItemHolder*)this;
  }

private:
  T* head_;
  T* tail_;
  uint64_t size_;
};



}//endif

#endif //_COMPONENT_DOUBLELINKEDLIST_H_
