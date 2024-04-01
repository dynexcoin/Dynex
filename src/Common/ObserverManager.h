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

#include <algorithm>
#include <mutex>
#include <vector>

namespace Tools {

template<typename T>
class ObserverManager {
public:
  bool add(T* observer) {
    std::unique_lock<std::mutex> lock(m_observersMutex);
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (m_observers.end() == it) {
      m_observers.push_back(observer);
      return true;
    } else {
      return false;
    }
  }

  bool remove(T* observer) {
    std::unique_lock<std::mutex> lock(m_observersMutex);

    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (m_observers.end() == it) {
      return false;
    } else {
      m_observers.erase(it);
      return true;
    }
  }

  void clear() {
    std::unique_lock<std::mutex> lock(m_observersMutex);
    m_observers.clear();
  }

#if defined(_MSC_VER)
  template<typename F>
  void notify(F notification) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)();
    }
  }

  template<typename F, typename Arg0>
  void notify(F notification, const Arg0& arg0) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0);
    }
  }

  template<typename F, typename Arg0, typename Arg1>
  void notify(F notification, const Arg0& arg0, const Arg1& arg1) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0, arg1);
    }
  }

  template<typename F, typename Arg0, typename Arg1, typename Arg2>
  void notify(F notification, const Arg0& arg0, const Arg1& arg1, const Arg2& arg2) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0, arg1, arg2);
    }
  }

  template<typename F, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
  void notify(F notification, const Arg0& arg0, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0, arg1, arg2, arg3);
    }
  }

  template<typename F, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void notify(F notification, const Arg0& arg0, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0, arg1, arg2, arg3, arg4);
    }
  }

  template<typename F, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
  void notify(F notification, const Arg0& arg0, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(arg0, arg1, arg2, arg3, arg4, arg5);
    }
  }

#else

  template<typename F, typename... Args>
  void notify(F notification, Args... args) {
    std::vector<T*> observersCopy;
    {
      std::unique_lock<std::mutex> lock(m_observersMutex);
      observersCopy = m_observers;
    }

    for (T* observer : observersCopy) {
      (observer->*notification)(args...);
    }
  }
#endif

private:
  std::vector<T*> m_observers;
  std::mutex m_observersMutex;
};

}
