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

#include <thread>
#include <System/RemoteContext.h>
#include <System/Dispatcher.h>
#include <System/ContextGroup.h>
#include <System/Event.h>
#include <System/InterruptedException.h>
#include <System/Timer.h>
#include <gtest/gtest.h>

using namespace System;

class RemoteContextTests : public testing::Test {
public:
  Dispatcher dispatcher;
};


TEST_F(RemoteContextTests, getReturnsResult) {
  RemoteContext<int> context(dispatcher, [&] { 
    return 2; 
  });

  ASSERT_EQ(2, context.get());
}

TEST_F(RemoteContextTests, getRethrowsException) {
  RemoteContext<> context(dispatcher, [&] {
    throw std::string("Hi there!"); 
  });

  ASSERT_THROW(context.get(), std::string);
}

TEST_F(RemoteContextTests, destructorIgnoresException) {
  ASSERT_NO_THROW(RemoteContext<>(dispatcher, [&] {
    throw std::string("Hi there!");
  }));
}

TEST_F(RemoteContextTests, canBeUsedWithoutObject) {
  ASSERT_EQ(42, RemoteContext<int>(dispatcher, [&] { return 42; }).get());
}

TEST_F(RemoteContextTests, interruptIsInterruptingWait) {
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    RemoteContext<> context(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    ASSERT_NO_THROW(context.wait());
    ASSERT_TRUE(dispatcher.interrupted());
  });

  cg.interrupt();
  cg.wait();
}

TEST_F(RemoteContextTests, interruptIsInterruptingGet) {
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    RemoteContext<> context(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    ASSERT_NO_THROW(context.wait());
    ASSERT_TRUE(dispatcher.interrupted());
  });

  cg.interrupt();
  cg.wait();
}

TEST_F(RemoteContextTests, destructorIgnoresInterrupt) {
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    ASSERT_NO_THROW(RemoteContext<>(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }));
  });

  cg.interrupt();
  cg.wait();
}

TEST_F(RemoteContextTests, canExecuteOtherContextsWhileWaiting) {
  auto start = std::chrono::high_resolution_clock::now();
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    RemoteContext<> context(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
  });
  cg.spawn([&] {
    System::Timer(dispatcher).sleep(std::chrono::milliseconds(50));
    auto end = std::chrono::high_resolution_clock::now();
    ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 50);
    ASSERT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 100);
  });

  cg.wait();
}

TEST_F(RemoteContextTests, waitMethodWaitsForContexCompletion) {
  auto start = std::chrono::high_resolution_clock::now();
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    RemoteContext<> context(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
  });

  cg.wait();
  auto end = std::chrono::high_resolution_clock::now();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
}

TEST_F(RemoteContextTests, waitMethodWaitsForContexCompletionOnInterrupt) {
  auto start = std::chrono::high_resolution_clock::now();
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    RemoteContext<> context(dispatcher, [&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
  });

  cg.interrupt();
  cg.wait();
  auto end = std::chrono::high_resolution_clock::now();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
}

