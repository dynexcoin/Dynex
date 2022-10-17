// Copyright (c) 2021-2022, Dynex Developers
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this project are originally copyright by:
// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#pragma once

namespace CryptoNote {

//Value must have public method IntrusiveLinkedList<Value>::hook& getHook()
template<class Value> class IntrusiveLinkedList {
public:
  class hook {
  public:
    friend class IntrusiveLinkedList<Value>;

    hook();
  private:
    Value* prev;
    Value* next;
    bool used;
  };

  class iterator {
  public:
    iterator(Value* value);

    bool operator!=(const iterator& other) const;
    bool operator==(const iterator& other) const;
    iterator& operator++();
    iterator operator++(int);
    iterator& operator--();
    iterator operator--(int);

    Value& operator*() const;
    Value* operator->() const;

  private:
    Value* currentElement;
  };

  IntrusiveLinkedList();

  bool insert(Value& value);
  bool remove(Value& value);

  bool empty() const;

  iterator begin();
  iterator end();
private:
  Value* head;
  Value* tail;
};

template<class Value>
IntrusiveLinkedList<Value>::IntrusiveLinkedList() : head(nullptr), tail(nullptr) {}

template<class Value>
bool IntrusiveLinkedList<Value>::insert(Value& value) {
  if (!value.getHook().used) {
    if (head == nullptr) {
      head = &value;
      tail = head;
      value.getHook().prev = nullptr;
    } else {
      tail->getHook().next = &value;
      value.getHook().prev = tail;
      tail = &value;
    }

    value.getHook().next = nullptr;
    value.getHook().used = true;
    return true;
  } else {
    return false;
  }
}

template<class Value>
bool IntrusiveLinkedList<Value>::remove(Value& value) {
  if (value.getHook().used && head != nullptr) {
    Value* toRemove = &value;
    Value* current = head;
    while (current->getHook().next != nullptr) {
      if (toRemove == current) {
        break;
      }

      current = current->getHook().next;
    }

    if (toRemove == current) {
      if (current->getHook().prev == nullptr) {
        assert(current == head);
        head = current->getHook().next;

        if (head != nullptr) {
          head->getHook().prev = nullptr;
        } else {
          tail = nullptr;
        }
      } else {
        current->getHook().prev->getHook().next = current->getHook().next;
        if (current->getHook().next != nullptr) {
          current->getHook().next->getHook().prev = current->getHook().prev;
        } else {
          tail = current->getHook().prev;
        }
      }

      current->getHook().prev = nullptr;
      current->getHook().next = nullptr;
      current->getHook().used = false;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

template<class Value>
bool IntrusiveLinkedList<Value>::empty() const {
  return head == nullptr;
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator IntrusiveLinkedList<Value>::begin() {
  return iterator(head);
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator IntrusiveLinkedList<Value>::end() {
  return iterator(nullptr);
}

template<class Value>
IntrusiveLinkedList<Value>::hook::hook() : prev(nullptr), next(nullptr), used(false) {}

template<class Value>
IntrusiveLinkedList<Value>::iterator::iterator(Value* value) : currentElement(value) {}

template<class Value>
bool IntrusiveLinkedList<Value>::iterator::operator!=(const typename IntrusiveLinkedList<Value>::iterator& other) const {
  return currentElement != other.currentElement;
}

template<class Value>
bool IntrusiveLinkedList<Value>::iterator::operator==(const typename IntrusiveLinkedList<Value>::iterator& other) const {
  return currentElement == other.currentElement;
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator& IntrusiveLinkedList<Value>::iterator::operator++() {
  assert(currentElement != nullptr);
  currentElement = currentElement->getHook().next;
  return *this;
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator IntrusiveLinkedList<Value>::iterator::operator++(int) {
  IntrusiveLinkedList<Value>::iterator copy = *this;

  assert(currentElement != nullptr);
  currentElement = currentElement->getHook().next;
  return copy;
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator& IntrusiveLinkedList<Value>::iterator::operator--() {
  assert(currentElement != nullptr);
  currentElement = currentElement->getHook().prev;
  return *this;
}

template<class Value>
typename IntrusiveLinkedList<Value>::iterator IntrusiveLinkedList<Value>::iterator::operator--(int) {
  IntrusiveLinkedList<Value>::iterator copy = *this;

  assert(currentElement != nullptr);
  currentElement = currentElement->getHook().prev;
  return copy;
}

template<class Value>
Value& IntrusiveLinkedList<Value>::iterator::operator*() const {
  assert(currentElement != nullptr);

  return *currentElement; 
}

template<class Value>
Value* IntrusiveLinkedList<Value>::iterator::operator->() const {
  return currentElement; 
}

}
