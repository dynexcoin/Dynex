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


#include "ContextGroup.h"
#include <cassert>

namespace System {

ContextGroup::ContextGroup(Dispatcher& dispatcher) : dispatcher(&dispatcher) {
  contextGroup.firstContext = nullptr;
}

ContextGroup::ContextGroup(ContextGroup&& other) : dispatcher(other.dispatcher) {
  if (dispatcher != nullptr) {
    assert(other.contextGroup.firstContext == nullptr);
    contextGroup.firstContext = nullptr;
    other.dispatcher = nullptr;
  }
}

ContextGroup::~ContextGroup() {
  if (dispatcher != nullptr) {
    interrupt();
    wait();
  }
}

ContextGroup& ContextGroup::operator=(ContextGroup&& other) {
  assert(dispatcher == nullptr || contextGroup.firstContext == nullptr);
  dispatcher = other.dispatcher;
  if (dispatcher != nullptr) {
    assert(other.contextGroup.firstContext == nullptr);
    contextGroup.firstContext = nullptr;
    other.dispatcher = nullptr;
  }

  return *this;
}

void ContextGroup::interrupt() {
  assert(dispatcher != nullptr);
  for (NativeContext* context = contextGroup.firstContext; context != nullptr; context = context->groupNext) {
    dispatcher->interrupt(context);
  }
}

void ContextGroup::spawn(std::function<void()>&& procedure) {
  assert(dispatcher != nullptr);
  NativeContext& context = dispatcher->getReusableContext();
  if (contextGroup.firstContext != nullptr) {
    context.groupPrev = contextGroup.lastContext;
    assert(contextGroup.lastContext->groupNext == nullptr);
    contextGroup.lastContext->groupNext = &context;
  } else {
    context.groupPrev = nullptr;
    contextGroup.firstContext = &context;
    contextGroup.firstWaiter = nullptr;
  }

  context.interrupted = false;
  context.group = &contextGroup;
  context.groupNext = nullptr;
  context.procedure = std::move(procedure);
  contextGroup.lastContext = &context;
  dispatcher->pushContext(&context);
}

void ContextGroup::wait() {
  if (contextGroup.firstContext != nullptr) {
    NativeContext* context = dispatcher->getCurrentContext();
    context->next = nullptr;
    if (contextGroup.firstWaiter != nullptr) {
      assert(contextGroup.lastWaiter->next == nullptr);
      contextGroup.lastWaiter->next = context;
    } else {
      contextGroup.firstWaiter = context;
    }

    contextGroup.lastWaiter = context;
    dispatcher->dispatch();
    assert(context == dispatcher->getCurrentContext());
  }
}

}
