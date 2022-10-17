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

#include <System/Dispatcher.h>
#include <System/Event.h>
#include <System/InterruptedException.h>

namespace System {

template<typename ResultType = void>
class Context {
public:
  Context(Dispatcher& dispatcher, std::function<ResultType()>&& target) : 
    dispatcher(dispatcher), target(std::move(target)), ready(dispatcher), bindingContext(dispatcher.getReusableContext()) {
    bindingContext.interrupted = false;
    bindingContext.groupNext = nullptr;
    bindingContext.groupPrev = nullptr;
    bindingContext.group = nullptr;
    bindingContext.procedure = [this] {
      try {
        new(resultStorage) ResultType(this->target());
      } catch (...) {
        exceptionPointer = std::current_exception();
      }

      ready.set();
    };

    dispatcher.pushContext(&bindingContext);
  }

  Context(const Context&) = delete;  
  Context& operator=(const Context&) = delete;

  ~Context() {
    interrupt();
    wait();
    dispatcher.pushReusableContext(bindingContext);
  }

  ResultType& get() {
    wait();
    if (exceptionPointer != nullptr) {
      std::rethrow_exception(exceptionPointer);
    }

    return *reinterpret_cast<ResultType*>(resultStorage);
  }

  void interrupt() {
    dispatcher.interrupt(&bindingContext);
  }

  void wait() {
    for (;;) {
      try {
        ready.wait();
        break;
      } catch (InterruptedException&) {
        interrupt();
      }
    }
  }

private:
  uint8_t resultStorage[sizeof(ResultType)];
  Dispatcher& dispatcher;
  std::function<ResultType()> target;
  Event ready;
  NativeContext& bindingContext;
  std::exception_ptr exceptionPointer;
};

template<>
class Context<void> {
public:
  Context(Dispatcher& dispatcher, std::function<void()>&& target) :
    dispatcher(dispatcher), target(std::move(target)), ready(dispatcher), bindingContext(dispatcher.getReusableContext()) {
    bindingContext.interrupted = false;
    bindingContext.groupNext = nullptr;
    bindingContext.groupPrev = nullptr;
    bindingContext.group = nullptr;
    bindingContext.procedure = [this] {
      try {
        this->target();
      } catch (...) {
        exceptionPointer = std::current_exception();
      }

      ready.set();
    };

    dispatcher.pushContext(&bindingContext);
  }

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  ~Context() {
    interrupt();
    wait();
    dispatcher.pushReusableContext(bindingContext);
  }

  void get() {
    wait();
    if (exceptionPointer != nullptr) {
      std::rethrow_exception(exceptionPointer);
    }
  }

  void interrupt() {
    dispatcher.interrupt(&bindingContext);
  }

  void wait() {
    for (;;) {
      try {
        ready.wait();
        break;
      } catch (InterruptedException&) {
        interrupt();
      }
    }
  }

private:
  Dispatcher& dispatcher;
  std::function<void()> target;
  Event ready;
  NativeContext& bindingContext;
  std::exception_ptr exceptionPointer;
};

}
