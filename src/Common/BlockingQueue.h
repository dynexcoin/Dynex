// Copyright (c) 2021-2023, Dynex Developers
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
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#pragma once 

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

template < typename T, typename Container = std::deque<T> >
class BlockingQueue {
public:

  typedef BlockingQueue<T, Container> ThisType;

  BlockingQueue(size_t maxSize = 1) : 
    m_maxSize(maxSize), m_closed(false) {}

  template <typename TT>
  bool push(TT&& v) {
    std::unique_lock<std::mutex> lk(m_mutex);

    while (!m_closed && m_queue.size() >= m_maxSize) {
      m_haveSpace.wait(lk);
    }

    if (m_closed) {
      return false;
    }

    m_queue.push_back(std::forward<TT>(v));
    m_haveData.notify_one();
    return true;
  }

  bool pop(T& v) {
    std::unique_lock<std::mutex> lk(m_mutex);

    while (m_queue.empty()) {
      if (m_closed) {
        // all data has been processed, queue is closed
        return false;
      }
      m_haveData.wait(lk);
    }
   
    v = std::move(m_queue.front());
    m_queue.pop_front();

    // we can have several waiting threads to unblock
    if (m_closed && m_queue.empty())
      m_haveSpace.notify_all();
    else
      m_haveSpace.notify_one();

    return true;
  }

  void close(bool wait = false) {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_closed = true;
    m_haveData.notify_all(); // wake up threads in pop()
    m_haveSpace.notify_all();

    if (wait) {
      while (!m_queue.empty()) {
        m_haveSpace.wait(lk);
      }
    }
  }

  size_t size() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_queue.size();
  }

  size_t capacity() const {
    return m_maxSize;
  }

private:

  const size_t m_maxSize;
  Container m_queue;
  bool m_closed;
  
  std::mutex m_mutex;
  std::condition_variable m_haveData;
  std::condition_variable m_haveSpace;
};

template <typename QueueT>
class GroupClose {
public:

  GroupClose(QueueT& queue, size_t groupSize)
    : m_queue(queue), m_count(groupSize) {}

  void close() {
    if (m_count == 0)
      return;
    if (m_count.fetch_sub(1) == 1)
      m_queue.close();
  }

private:

  std::atomic<size_t> m_count;
  QueueT& m_queue;

};
