// Copyright (c) 2021-2024, Dynex Developers
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
// Copyright (c) 2011-2012, Kristian Nielsen and Monty Program Ab
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers

/*
  Implementation of async context spawning using Posix ucontext and swapcontext().
*/

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define _XOPEN_SOURCE 600
#include <ucontext.h>

#include "Dispatcher.h"
#include <cassert>
#include <string>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "ErrorMessage.h"


namespace System {

namespace {

/*
  The makecontext() only allows to pass integers into the created context :-(
  We want to pass pointers, so we do it this kinda hackish way.
  Anyway, it should work everywhere, and at least it does not break strict aliasing.
*/

union pass_void_ptr_as_2_int {
  int a[2];
  void *p;
};

struct ContextMakingData {
  void* uctx;
  Dispatcher* dispatcher;
};

class MutextGuard {
public:
  MutextGuard(pthread_mutex_t& _mutex) : mutex(_mutex) {
    auto ret = pthread_mutex_lock(&mutex);
    if (ret != 0) {
      throw std::runtime_error("MutextGuard::MutextGuard, pthread_mutex_lock failed, " + errorMessage(ret));
    }
  }

  ~MutextGuard() {
    pthread_mutex_unlock(&mutex);
  }

private:
  pthread_mutex_t& mutex;
};

const size_t STACK_SIZE = 512 * 1024;

}

static_assert(Dispatcher::SIZEOF_PTHREAD_MUTEX_T == sizeof(pthread_mutex_t), "invalid pthread mutex size");

Dispatcher::Dispatcher() : lastCreatedTimer(0) {
  std::string message;
  kqueue = ::kqueue();
  if (kqueue == -1) {
    message = "kqueue failed, " + lastErrorMessage();
  } else {
    mainContext.uctx = new ucontext_t;
    if (getcontext(static_cast<ucontext_t*>(mainContext.uctx)) == -1) {
      message = "getcontext failed, " + lastErrorMessage();
    } else {
      struct kevent event;
      EV_SET(&event, 0, EVFILT_USER, EV_ADD, NOTE_FFNOP, 0, NULL);
      if (kevent(kqueue, &event, 1, NULL, 0, NULL) == -1) {
        message = "kevent failed, " + lastErrorMessage();
      } else {
        if(pthread_mutex_init(reinterpret_cast<pthread_mutex_t*>(this->mutex), NULL) == -1) {
          message = "pthread_mutex_init failed, " + lastErrorMessage();
        } else {
          remoteSpawned = false;

          mainContext.interrupted = false;
          mainContext.group = &contextGroup;
          mainContext.groupPrev = nullptr;
          mainContext.groupNext = nullptr;
          contextGroup.firstContext = nullptr;
          contextGroup.lastContext = nullptr;
          contextGroup.firstWaiter = nullptr;
          contextGroup.lastWaiter = nullptr;
          mainContext.inExecutionQueue = false;
          currentContext = &mainContext;
          firstResumingContext = nullptr;
          firstReusableContext = nullptr;
          runningContextCount = 0;
          return;
        }
      }
    }

    auto result = close(kqueue);
    assert(result == 0);
  }

  throw std::runtime_error("Dispatcher::Dispatcher, " + message);
}

Dispatcher::~Dispatcher() {
  for (NativeContext* context = contextGroup.firstContext; context != nullptr; context = context->groupNext) {
    interrupt(context);
  }

  yield();
  assert(contextGroup.firstContext == nullptr);
  assert(contextGroup.firstWaiter == nullptr);
  assert(firstResumingContext == nullptr);
  assert(runningContextCount == 0);
  while (firstReusableContext != nullptr) {
    auto ucontext = static_cast<ucontext_t*>(firstReusableContext->uctx);
    auto stackPtr = static_cast<uint8_t*>(firstReusableContext->stackPtr);
    firstReusableContext = firstReusableContext->next;
    if (stackPtr) delete[] stackPtr;
    if (ucontext) delete ucontext;
  }
  auto result = close(kqueue);
  assert(result != -1);
  auto result2 = pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(this->mutex));
  assert(result2 != -1);
}

void Dispatcher::clear() {
  while (firstReusableContext != nullptr) {
    auto ucontext = static_cast<ucontext_t*>(firstReusableContext->uctx);
    auto stackPtr = static_cast<uint8_t*>(firstReusableContext->stackPtr);
    firstReusableContext = firstReusableContext->next;
    if (stackPtr) delete[] stackPtr;
    if (ucontext) delete ucontext;
  }
}

void Dispatcher::dispatch() {
  NativeContext* context;
  for (;;) {
    if (firstResumingContext != nullptr) {
      context = firstResumingContext;
      firstResumingContext = context->next;
      //assert(context->inExecutionQueue);
      context->inExecutionQueue = false;
      break;
    }

    if(remoteSpawned.load() == true) {
      MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
      while (!remoteSpawningProcedures.empty()) {
        spawn(std::move(remoteSpawningProcedures.front()));
        remoteSpawningProcedures.pop();
      }

      remoteSpawned = false;
      continue;
    }

    struct kevent event;
    int count = kevent(kqueue, NULL, 0, &event, 1, NULL);
    if (count == 1) {
      if (event.flags & EV_ERROR) {
        continue;
      }

      if (event.filter == EVFILT_USER && event.ident == 0) {
        struct kevent event;
        EV_SET(&event, 0, EVFILT_USER, EV_ADD | EV_DISABLE, NOTE_FFNOP, 0, NULL);
        if (kevent(kqueue, &event, 1, NULL, 0, NULL) == -1) {
          throw std::runtime_error("Dispatcher::dispatch, kevent failed, " + lastErrorMessage());
        }

        continue;
      }

      if (event.filter == EVFILT_WRITE) {
        event.flags = EV_DELETE | EV_DISABLE;
        kevent(kqueue, &event, 1, NULL, 0, NULL); // ignore error here
      }

      context = static_cast<OperationContext*>(event.udata)->context;
      break;
    }

    if (errno != EINTR) {
      throw std::runtime_error("Dispatcher::dispatch, kqueue failed, " + lastErrorMessage());
    } else {
      MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
      while (!remoteSpawningProcedures.empty()) {
        spawn(std::move(remoteSpawningProcedures.front()));
        remoteSpawningProcedures.pop();
      }

    }
  }

  if (context != currentContext) {
    ucontext_t* oldContext = static_cast<ucontext_t*>(currentContext->uctx);
    currentContext = context;
    if (swapcontext(oldContext, static_cast<ucontext_t*>(currentContext->uctx)) == -1) {
      throw std::runtime_error("Dispatcher::dispatch, swapcontext failed, " + lastErrorMessage());
    }
  }
}

NativeContext* Dispatcher::getCurrentContext() const {
  return currentContext;
}

void Dispatcher::interrupt() {
  interrupt(currentContext);
}

void Dispatcher::interrupt(NativeContext* context) {
  assert(context!=nullptr);
  if (!context->interrupted) {
    if (context->interruptProcedure != nullptr) {
      context->interruptProcedure();
      context->interruptProcedure = nullptr;
    } else {
      context->interrupted = true;
    }
  }
}

bool Dispatcher::interrupted() {
  if (currentContext->interrupted) {
    currentContext->interrupted = false;
    return true;
  }

  return false;
}

void Dispatcher::pushContext(NativeContext* context) {
  assert(context!=nullptr);
  if (context->inExecutionQueue)
    return;
  context->next = nullptr;
  context->inExecutionQueue = true;
  if (firstResumingContext != nullptr) {
    assert(lastResumingContext != nullptr);
    lastResumingContext->next = context;
  } else {
    firstResumingContext = context;
  }

  lastResumingContext = context;
}

void Dispatcher::remoteSpawn(std::function<void()>&& procedure) {
  MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
  remoteSpawningProcedures.push(std::move(procedure));
  if (remoteSpawned == false) {
    remoteSpawned = true;
    struct kevent event;
    EV_SET(&event, 0, EVFILT_USER, EV_ADD | EV_ENABLE, NOTE_FFCOPY | NOTE_TRIGGER, 0, NULL);
    if (kevent(kqueue, &event, 1, NULL, 0, NULL) == -1) {
      throw std::runtime_error("Dispatcher::remoteSpawn, kevent failed, " + lastErrorMessage());
    };
  }
}

void Dispatcher::spawn(std::function<void()>&& procedure) {
  NativeContext* context = &getReusableContext();
  if(contextGroup.firstContext != nullptr) {
    context->groupPrev = contextGroup.lastContext;
    assert(contextGroup.lastContext->groupNext == nullptr);
    contextGroup.lastContext->groupNext = context;
  } else {
    context->groupPrev = nullptr;
    contextGroup.firstContext = context;
    contextGroup.firstWaiter = nullptr;
  }

  context->interrupted = false;
  context->group = &contextGroup;
  context->groupNext = nullptr;
  context->procedure = std::move(procedure);
  contextGroup.lastContext = context;
  pushContext(context);
}

void Dispatcher::yield() {
  struct timespec zeroTimeout = { 0, 0 };
  int updatesCounter = 0;
  for (;;) {
    struct kevent events[16];
    struct kevent updates[16];
    int count = kevent(kqueue, updates, updatesCounter, events, 16, &zeroTimeout);
    if (count == 0) {
      break;
    }

    updatesCounter = 0;
    if (count > 0) {
      for (int i = 0; i < count; ++i) {
        if (events[i].flags & EV_ERROR) {
          continue;
        }

        if (events[i].filter == EVFILT_USER && events[i].ident == 0) {
          EV_SET(&updates[updatesCounter++], 0, EVFILT_USER, EV_ADD | EV_DISABLE, NOTE_FFNOP, 0, NULL);

          MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
          while (!remoteSpawningProcedures.empty()) {
            spawn(std::move(remoteSpawningProcedures.front()));
            remoteSpawningProcedures.pop();
          }

          remoteSpawned = false;
          continue;
        }

        static_cast<OperationContext*>(events[i].udata)->context->interruptProcedure = nullptr;
        pushContext(static_cast<OperationContext*>(events[i].udata)->context);
        if (events[i].filter == EVFILT_WRITE) {
          EV_SET(&updates[updatesCounter++], events[i].ident, EVFILT_WRITE, EV_DELETE | EV_DISABLE, 0, 0, NULL);
        }
      }
    } else {
      if (errno != EINTR) {
        throw std::runtime_error("Dispatcher::dispatch, kevent failed, " + lastErrorMessage());
      }
    }
  }

  if (firstResumingContext != nullptr) {
    pushContext(currentContext);
    dispatch();
  }
}

int Dispatcher::getKqueue() const {
  return kqueue;
}

NativeContext& Dispatcher::getReusableContext() {
  if(firstReusableContext == nullptr) {
   ucontext_t* newlyCreatedContext = new ucontext_t;
   if (getcontext(static_cast<ucontext_t*>(newlyCreatedContext)) == -1) {
     throw std::runtime_error("Dispatcher::getReusableContext, getcontext failed, " + lastErrorMessage());
   }
   uint8_t* stackPointer = new uint8_t[STACK_SIZE];
   static_cast<ucontext_t*>(newlyCreatedContext)->uc_stack.ss_sp = stackPointer;
   static_cast<ucontext_t*>(newlyCreatedContext)->uc_stack.ss_size = STACK_SIZE;
   static_cast<ucontext_t*>(newlyCreatedContext)->uc_link = NULL;

   ContextMakingData data{ newlyCreatedContext, this };
   union pass_void_ptr_as_2_int u;
   u.p = &data;
   makecontext(static_cast<ucontext_t*>(newlyCreatedContext), reinterpret_cast<void(*)()>(contextProcedureStatic), 2, u.a[0], u.a[1]);

   ucontext_t* oldContext = static_cast<ucontext_t*>(currentContext->uctx);
   if (swapcontext(oldContext, newlyCreatedContext) == -1) {
     throw std::runtime_error("Dispatcher::getReusableContext, swapcontext failed, " + lastErrorMessage());
   }

   assert(firstReusableContext != nullptr);
   assert(firstReusableContext->uctx == newlyCreatedContext);
   firstReusableContext->stackPtr = stackPointer;
  }

  NativeContext* context = firstReusableContext;
  firstReusableContext = firstReusableContext->next;
  return *context;
}

void Dispatcher::pushReusableContext(NativeContext& context) {
  context.next = firstReusableContext;
  firstReusableContext = &context;
  --runningContextCount;
}

int Dispatcher::getTimer() {
  int timer;
  if (timers.empty()) {
    timer = ++lastCreatedTimer;
  } else {
    timer = timers.top();
    timers.pop();
  }

  return timer;
}

void Dispatcher::pushTimer(int timer) {
  timers.push(timer);
}

void Dispatcher::contextProcedure(void* ucontext) {
  assert(firstReusableContext == nullptr);
  NativeContext context;
  context.uctx = ucontext;
  context.interrupted = false;
  context.next = nullptr;
  context.inExecutionQueue = false;
  firstReusableContext = &context;
  ucontext_t* oldContext = static_cast<ucontext_t*>(context.uctx);
  if (swapcontext(oldContext, static_cast<ucontext_t*>(currentContext->uctx)) == -1) {
    throw std::runtime_error("Dispatcher::contextProcedure, swapcontext failed, " + lastErrorMessage());
  }

  for (;;) {
    ++runningContextCount;
    try {
      context.procedure();
    } catch(std::exception&) {
    }

    if (context.group != nullptr) {
      if (context.groupPrev != nullptr) {
        assert(context.groupPrev->groupNext == &context);
        context.groupPrev->groupNext = context.groupNext;
        if (context.groupNext != nullptr) {
          assert(context.groupNext->groupPrev == &context);
          context.groupNext->groupPrev = context.groupPrev;
        } else {
          assert(context.group->lastContext == &context);
          context.group->lastContext = context.groupPrev;
        }
      } else {
        assert(context.group->firstContext == &context);
        context.group->firstContext = context.groupNext;
        if (context.groupNext != nullptr) {
          assert(context.groupNext->groupPrev == &context);
          context.groupNext->groupPrev = nullptr;
        } else {
          assert(context.group->lastContext == &context);
          if (context.group->firstWaiter != nullptr) {
            if (firstResumingContext != nullptr) {
              assert(lastResumingContext->next == nullptr);
              lastResumingContext->next = context.group->firstWaiter;
            } else {
              firstResumingContext = context.group->firstWaiter;
            }

            lastResumingContext = context.group->lastWaiter;
            context.group->firstWaiter = nullptr;
          }
        }
      }

      pushReusableContext(context);
    }

    dispatch();
  }
}

void Dispatcher::contextProcedureStatic(int i0, int i1) {
  union pass_void_ptr_as_2_int u;
  u.a[0] = i0;
  u.a[1] = i1;
  ContextMakingData* data = static_cast<ContextMakingData*>(u.p);
  data->dispatcher->contextProcedure(data->uctx);
}

}
