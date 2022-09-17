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

#include "Timer.h"
#include <cassert>
#include <stdexcept>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "Dispatcher.h"
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>

namespace System {

Timer::Timer() : dispatcher(nullptr) {
}

Timer::Timer(Dispatcher& dispatcher) : dispatcher(&dispatcher), context(nullptr), timer(-1) {
}

Timer::Timer(Timer&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    timer = other.timer;
    context = nullptr;
    other.dispatcher = nullptr;
  }
}

Timer::~Timer() {
  assert(dispatcher == nullptr || context == nullptr);
}

Timer& Timer::operator=(Timer&& other) {
  assert(dispatcher == nullptr || context == nullptr);
  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    timer = other.timer;
    context = nullptr;
    other.dispatcher = nullptr;
    other.timer = -1;
  }

  return *this;
}

void Timer::sleep(std::chrono::nanoseconds duration) {
  assert(dispatcher != nullptr);
  assert(context == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  if(duration.count() == 0 ) {
    dispatcher->yield();
  } else {
    timer = dispatcher->getTimer();

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    itimerspec expires;
    expires.it_interval.tv_nsec = expires.it_interval.tv_sec = 0;
    expires.it_value.tv_sec = seconds.count();
    expires.it_value.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds).count();
    timerfd_settime(timer, 0, &expires, NULL);

    ContextPair contextPair;
    OperationContext timerContext;
    timerContext.interrupted = false;
    timerContext.context = dispatcher->getCurrentContext();
    contextPair.writeContext = nullptr;
    contextPair.readContext = &timerContext;

    epoll_event timerEvent;
    timerEvent.events = EPOLLIN | EPOLLONESHOT;
    timerEvent.data.ptr = &contextPair;

    if (epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, timer, &timerEvent) == -1) {
      throw std::runtime_error("Timer::sleep, epoll_ctl failed, " + lastErrorMessage());
    }
    dispatcher->getCurrentContext()->interruptProcedure = [&]() {
        assert(dispatcher != nullptr);
        assert(context != nullptr);
        OperationContext* timerContext = static_cast<OperationContext*>(context);
        if (!timerContext->interrupted) {
          uint64_t value = 0;
          if(::read(timer, &value, sizeof value) == -1 ){
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
              timerContext->interrupted = true;
              dispatcher->pushContext(timerContext->context);
            } else {
              throw std::runtime_error("Timer::interrupt, read failed, "  + lastErrorMessage());
            }
          } else {
            assert(value>0);
            dispatcher->pushContext(timerContext->context);
          }

          epoll_event timerEvent;
          timerEvent.events = 0;
          timerEvent.data.ptr = nullptr;

          if (epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, timer, &timerEvent) == -1) {
            throw std::runtime_error("Timer::interrupt, epoll_ctl failed, " + lastErrorMessage());
          }
        }
    };

    context = &timerContext;
    dispatcher->dispatch();
    dispatcher->getCurrentContext()->interruptProcedure = nullptr;
    assert(dispatcher != nullptr);
    assert(timerContext.context == dispatcher->getCurrentContext());
    assert(contextPair.writeContext == nullptr);
    assert(context == &timerContext);
    context = nullptr;
    timerContext.context = nullptr;
    dispatcher->pushTimer(timer);
    if (timerContext.interrupted) {
      throw InterruptedException();
    }
  }
}

}
