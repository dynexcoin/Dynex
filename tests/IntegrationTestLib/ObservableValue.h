// Copyright (c) 2021-2022, The TuringX Project
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
// Parts of this file are originally copyright (c) 2012-2016 The Cryptonote developers

#pragma once

#include <mutex>
#include <condition_variable>

template <typename T>
class ObservableValueBase {
public:
  ObservableValueBase(std::mutex& mtx, std::condition_variable& cv, const T defaultValue = T()) :
    m_mutex(mtx), m_cv(cv), m_value(defaultValue), m_updated(false) {
  }

  void set(T value) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_value = value;
    m_updated = true;
    m_cv.notify_all();
  }

  void increment() {
    std::lock_guard<std::mutex> lk(m_mutex);
    ++m_value;
    m_updated = true;
    m_cv.notify_all();
  }

  T get() {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_value;
  }

  bool waitFor(std::chrono::milliseconds ms, T& value) {
    std::unique_lock<std::mutex> lk(m_mutex);

    if (m_cv.wait_for(lk, ms, [this] { return m_updated; })) {
      value = m_value;
      m_updated = false;
      return true;
    }

    return false;
  }

  T wait() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv.wait(lk, [this] { return m_updated; });
    m_updated = false;
    return m_value;
  }

private:

  std::mutex& m_mutex;
  std::condition_variable& m_cv;

  T m_value;
  bool m_updated;
};
