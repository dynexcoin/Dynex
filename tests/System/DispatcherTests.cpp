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

#include <thread> //dm
#include <future>
#include <System/Context.h>
#include <System/Dispatcher.h>
#include <System/Event.h>
#include <System/Timer.h>
#include <gtest/gtest.h>

using namespace System;

class DispatcherTests : public testing::Test {
public:
  Dispatcher dispatcher;
};

TEST_F(DispatcherTests, clearRemainsDispatcherWorkable) {
  dispatcher.clear();
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, clearRemainsDispatcherWorkableAfterAsyncOperation) {
  bool spawn1Done = false;
  bool spawn2Done = false;
  Context<> context(dispatcher, [&]() {
    spawn1Done = true;
  });
  
  dispatcher.yield();
  ASSERT_TRUE(spawn1Done);
  dispatcher.clear();
  Context<> contextSecond(dispatcher, [&]() {
    spawn2Done = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawn2Done);
}

TEST_F(DispatcherTests, clearCalledFromSpawnRemainsDispatcherWorkable) {
  bool spawn1Done = false;
  bool spawn2Done = false;
  Context<> context(dispatcher, [&]() {
    dispatcher.clear();
    spawn1Done = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawn1Done);
  Context<> contextSecond(dispatcher, [&]() {
    spawn2Done = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawn2Done);
}

TEST_F(DispatcherTests, timerIsHandledOnlyAfterAllSpawnedTasksAreHandled) {
  Event event1(dispatcher);
  Event event2(dispatcher);
  Context<> context(dispatcher, [&]() {
    event1.set();
    Timer(dispatcher).sleep(std::chrono::milliseconds(1));
    event2.set();
  });

  dispatcher.yield();
  ASSERT_TRUE(event1.get());
  ASSERT_FALSE(event2.get());
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  dispatcher.pushContext(dispatcher.getCurrentContext());
  dispatcher.dispatch();
  ASSERT_FALSE(event2.get());
  dispatcher.yield();
  ASSERT_TRUE(event2.get());
}

TEST_F(DispatcherTests, dispatchKeepsSpawnOrder) {
  std::deque<size_t> executionOrder;
  std::deque<size_t> expectedOrder = { 1, 2 };
  Context<> context(dispatcher, [&]() {
    executionOrder.push_back(1);
  });

  Context<> contextSecond(dispatcher, [&]() {
    executionOrder.push_back(2);
  });

  dispatcher.pushContext(dispatcher.getCurrentContext());
  dispatcher.dispatch();
  ASSERT_EQ(executionOrder, expectedOrder);
}

TEST_F(DispatcherTests, dispatchKeepsSpawnOrderWithNesting) {
  std::deque<size_t> executionOrder;
  std::deque<size_t> expectedOrder = { 1, 2, 3, 4 };
  auto mainContext = dispatcher.getCurrentContext();
  Context<> context(dispatcher, [&]() {
    executionOrder.push_back(1);
    Context<> context(dispatcher, [&]() {
      executionOrder.push_back(3);
    });
  });

  Context<> contextSecond(dispatcher, [&]() {
    executionOrder.push_back(2);
    Context<> context(dispatcher, [&]() {
      executionOrder.push_back(4);
      dispatcher.pushContext(mainContext);
    });
  });

  dispatcher.dispatch();
  ASSERT_EQ(executionOrder, expectedOrder);
}

TEST_F(DispatcherTests, dispatchKeepsSpawnResumingOrder) {
  std::deque<size_t> executionOrder;
  std::deque<size_t> expectedOrder = { 1, 2, 3, 4 };
  std::vector<NativeContext*> contexts;
  Context<> context(dispatcher, [&]() {
    executionOrder.push_back(1);
    contexts.push_back(dispatcher.getCurrentContext());
    dispatcher.dispatch();
    executionOrder.push_back(3);
  });

  Context<> contextSecond(dispatcher, [&]() {
    executionOrder.push_back(2);
    contexts.push_back(dispatcher.getCurrentContext());
    dispatcher.dispatch();
    executionOrder.push_back(4);
  });

  dispatcher.pushContext(dispatcher.getCurrentContext());
  dispatcher.dispatch();
  for (auto& ctx : contexts) {
    dispatcher.pushContext(ctx);
  }

  dispatcher.pushContext(dispatcher.getCurrentContext());
  dispatcher.dispatch();
  ASSERT_EQ(executionOrder, expectedOrder);
}

TEST_F(DispatcherTests, getCurrentContextDiffersForParallelSpawn) {
  void* ctx1 = nullptr;
  void* ctx2 = nullptr;
  Context<> context(dispatcher, [&]() {
    ctx1 = dispatcher.getCurrentContext();
  });

  Context<> contextSecond(dispatcher, [&]() {
    ctx2 = dispatcher.getCurrentContext();
  });

  dispatcher.yield();
  ASSERT_NE(ctx1, nullptr);
  ASSERT_NE(ctx2, nullptr);
  ASSERT_NE(ctx1, ctx2);
}

TEST_F(DispatcherTests, getCurrentContextSameForSequentialSpawn) {
  void* ctx1 = nullptr;
  void* ctx2 = nullptr;
  Context<> context(dispatcher, [&]() {
    ctx1 = dispatcher.getCurrentContext();
    dispatcher.yield();
    ctx2 = dispatcher.getCurrentContext();
  });

  dispatcher.yield();
  dispatcher.yield();
  ASSERT_NE(ctx1, nullptr);
  ASSERT_EQ(ctx1, ctx2);
}

TEST_F(DispatcherTests, pushedContextMustGoOn) {
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
  });

  dispatcher.pushContext(dispatcher.getCurrentContext());
  dispatcher.dispatch();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, pushedContextMustGoOnFromNestedSpawns) {
  bool spawnDone = false;
  auto mainContext = dispatcher.getCurrentContext();
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
    dispatcher.pushContext(mainContext);
  });

  dispatcher.dispatch();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, remoteSpawnActuallySpawns) {
  Event remoteSpawnDone(dispatcher);
  auto remoteSpawnThread = std::thread([&] {
    dispatcher.remoteSpawn([&]() {
      remoteSpawnDone.set();
    });
  });

  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }

  dispatcher.yield();
  ASSERT_TRUE(remoteSpawnDone.get());
}

TEST_F(DispatcherTests, remoteSpawnActuallySpawns2) {
  Event remoteSpawnDone(dispatcher);
  auto remoteSpawnThread = std::thread([&] { 
    dispatcher.remoteSpawn([&]() { 
      remoteSpawnDone.set(); 
    }); 
  });

  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }

  Timer(dispatcher).sleep(std::chrono::milliseconds(3));
  ASSERT_TRUE(remoteSpawnDone.get());
}

TEST_F(DispatcherTests, remoteSpawnActuallySpawns3) {
  Event remoteSpawnDone(dispatcher);
  auto mainCtx = dispatcher.getCurrentContext();
  auto remoteSpawnThread = std::thread([&, this] {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    dispatcher.remoteSpawn([&, this]() {
      remoteSpawnDone.set();
      dispatcher.pushContext(mainCtx);
    });
  });

  dispatcher.dispatch();
  ASSERT_TRUE(remoteSpawnDone.get());
  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }
}

TEST_F(DispatcherTests, remoteSpawnSpawnsProcedureInDispatcherThread) {
  Event remoteSpawnDone(dispatcher);
  auto mainSpawnThrId = std::this_thread::get_id();
  decltype(mainSpawnThrId) remoteSpawnThrId;
  auto remoteSpawnThread = std::thread([&] {
    dispatcher.remoteSpawn([&]() {
      remoteSpawnThrId = std::this_thread::get_id();
      remoteSpawnDone.set();
    });
  });

  remoteSpawnDone.wait();
  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }

  ASSERT_EQ(mainSpawnThrId, remoteSpawnThrId);
}

TEST_F(DispatcherTests, remoteSpawnSpawnsProcedureAndKeepsOrder) {
  Event remoteSpawnDone(dispatcher);
  std::deque<size_t> executionOrder;
  std::deque<size_t> expectedOrder = { 1, 2 };
  auto remoteSpawnThread = std::thread([&] {
    dispatcher.remoteSpawn([&]() {
      executionOrder.push_back(1);
    });

    dispatcher.remoteSpawn([&]() {
      executionOrder.push_back(2);
      remoteSpawnDone.set();
    });
  });

  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }

  remoteSpawnDone.wait();
  ASSERT_EQ(executionOrder, expectedOrder);
}

TEST_F(DispatcherTests, remoteSpawnActuallyWorksParallel) {
  Event remoteSpawnDone(dispatcher);
  auto remoteSpawnThread = std::thread([&] {
    dispatcher.remoteSpawn([&]() {
      remoteSpawnDone.set();
    });
  });

  Timer(dispatcher).sleep(std::chrono::milliseconds(100));
  ASSERT_TRUE(remoteSpawnDone.get());

  if (remoteSpawnThread.joinable()) {
    remoteSpawnThread.join();
  }
}

TEST_F(DispatcherTests, spawnActuallySpawns) {
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, spawnJustSpawns) {
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
  });

  ASSERT_FALSE(spawnDone);
  dispatcher.yield();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, yieldReturnsIfNothingToSpawn) {
  dispatcher.yield();
}

TEST_F(DispatcherTests, yieldReturnsAfterExecutionOfSpawnedProcedures) {
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    spawnDone = true;
  });

  dispatcher.yield();
  ASSERT_TRUE(spawnDone);
}

TEST_F(DispatcherTests, yieldReturnsAfterExecutionOfIO) {
  Context<> context(dispatcher, [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    dispatcher.yield();
  });

  Timer(dispatcher).sleep(std::chrono::milliseconds(1));
  dispatcher.yield();
  SUCCEED();
}

TEST_F(DispatcherTests, yieldExecutesIoOnItsFront) {
  bool spawnDone = false;
  Context<> context(dispatcher, [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    dispatcher.yield();
    spawnDone = true;
  });

  Timer(dispatcher).sleep(std::chrono::milliseconds(1));
  ASSERT_FALSE(spawnDone);
  dispatcher.yield();
  ASSERT_TRUE(spawnDone);
}
