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

#include <System/Context.h>
#include <System/Dispatcher.h>
#include <System/Event.h>
#include <System/InterruptedException.h>
#include <System/Timer.h>
#include <gtest/gtest.h>

using namespace System;

TEST(ContextTests, getReturnsResult) {
  Dispatcher dispatcher;
  Context<int> context(dispatcher, [&] { 
    return 2; 
  });

  ASSERT_EQ(2, context.get());
}

TEST(ContextTests, getRethrowsException) {
  Dispatcher dispatcher;
  Context<> context(dispatcher, [&] {
    throw std::string("Hi there!"); 
  });

  ASSERT_THROW(context.get(), std::string);
}

TEST(ContextTests, destructorIgnoresException) {
  Dispatcher dispatcher;
  ASSERT_NO_THROW(Context<>(dispatcher, [&] {
    throw std::string("Hi there!");
  }));
}

TEST(ContextTests, interruptIsInterrupting) {
  Dispatcher dispatcher;
  Context<> context(dispatcher, [&] {
    if (dispatcher.interrupted()) {
      throw InterruptedException();
    }
  });

  context.interrupt();
  ASSERT_THROW(context.get(), InterruptedException);
}

TEST(ContextTests, getChecksInterruption) {
  Dispatcher dispatcher;
  Event event(dispatcher);
  Context<int> context1(dispatcher, [&] {
    event.wait();
    if (dispatcher.interrupted()) {
      return 11;
    }

    return 10;
  });

  Context<int> context2(dispatcher, [&] {
    event.set();
    return context1.get();
  });

  context2.interrupt();
  ASSERT_EQ(11, context2.get());
}

TEST(ContextTests, getIsInterruptible) {
  Dispatcher dispatcher;
  Event event1(dispatcher);
  Event event2(dispatcher);
  Context<int> context1(dispatcher, [&] {
    event2.wait();
    if (dispatcher.interrupted()) {
      return 11;
    }

    return 10;
  });

  Context<int> context2(dispatcher, [&] {
    event1.set();
    return context1.get();
  });

  event1.wait();
  context2.interrupt();
  event2.set();
  ASSERT_EQ(11, context2.get());
}

TEST(ContextTests, destructorInterrupts) {
  Dispatcher dispatcher;
  bool interrupted = false;
  {
    Context<> context(dispatcher, [&] {
      if (dispatcher.interrupted()) {
        interrupted = true;
      }
    });
  }

  ASSERT_TRUE(interrupted);
}
