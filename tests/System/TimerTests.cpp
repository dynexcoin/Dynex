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
#include <System/Context.h>
#include <System/Dispatcher.h>
#include <System/ContextGroup.h>
#include <System/Event.h>
#include <System/InterruptedException.h>
#include <System/Timer.h>
#include <gtest/gtest.h>

using namespace System;

class TimerTests : public testing::Test {
public:
  TimerTests() : contextGroup(dispatcher) {
  }

  Dispatcher dispatcher;
  ContextGroup contextGroup;
};

TEST_F(TimerTests, timerIsWorking) {
  bool done = false;
  contextGroup.spawn([&]() {
    done = true;
  });

  ASSERT_FALSE(done);
  Timer(dispatcher).sleep(std::chrono::milliseconds(10));
  ASSERT_TRUE(done);
}

TEST_F(TimerTests, movedTimerIsWorking) {
  Timer t{Timer{dispatcher}};
  bool done = false;
  contextGroup.spawn([&]() {
    done = true;
  });

  ASSERT_FALSE(done);
  t.sleep(std::chrono::milliseconds(10));
  ASSERT_TRUE(done);
}

TEST_F(TimerTests, movedAndStoopedTimerIsWorking) {
  contextGroup.spawn([&] {
    Timer src(dispatcher);
    contextGroup.interrupt();
    Timer t(std::move(src));

    ASSERT_ANY_THROW(t.sleep(std::chrono::milliseconds(1)));
  });
}

TEST_F(TimerTests, doubleTimerTest) {
  auto begin = std::chrono::high_resolution_clock::now();
  Event first(dispatcher);
  Event second(dispatcher);
  Context<> context(dispatcher, [&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(100));
    first.set();
  });

  Context<> contextSecond(dispatcher, [&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(200));
    second.set();
  });

  first.wait();
  second.wait();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count(), 150);
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(275));
}

TEST_F(TimerTests, doubleTimerTestGroup) {
  auto begin = std::chrono::high_resolution_clock::now();
  Event first(dispatcher);
  Event second(dispatcher);
  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(100));
    first.set();
  });

  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(200));
    second.set();
  });

  first.wait();
  second.wait();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count(), 150);
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(250));
}

TEST_F(TimerTests, doubleTimerTestGroupWait) {
  auto begin = std::chrono::high_resolution_clock::now();
  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(100));
  });

  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(200));
  });

  contextGroup.wait();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count(), 150);
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(250));
}

TEST_F(TimerTests, doubleTimerTestTwoGroupsWait) {
  auto begin = std::chrono::high_resolution_clock::now();
  ContextGroup cg(dispatcher);
  cg.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(100));
  });

  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(200));
  });

  contextGroup.wait();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count(), 150);
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(275));
}

TEST_F(TimerTests, movedTimerIsWorking2) {
  bool done = false;
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    t = Timer{dispatcher};
    //contextGroup.spawn([&]() { done = true; });

    ASSERT_FALSE(done);
    t.sleep(std::chrono::milliseconds(10));
    ASSERT_TRUE(done);
  });

  contextGroup.spawn([&]() { 
    done = true; 
  });

  contextGroup.wait();
}

TEST_F(TimerTests, movedAndStoopedTimerIsWorking2) {
  contextGroup.spawn([&] {
    Timer src(dispatcher);
    contextGroup.interrupt();
    Timer t(dispatcher);
    t = std::move(src);

    ASSERT_ANY_THROW(t.sleep(std::chrono::milliseconds(1)));
  });
}

TEST_F(TimerTests, movedTimerIsTheSame) {
  contextGroup.spawn([&] {
    Timer timer(dispatcher);
    auto timerPtr1 = &timer;
    Timer srcEvent(dispatcher);
    timer = std::move(srcEvent);
    auto timerPtr2 = &timer;
    ASSERT_EQ(timerPtr1, timerPtr2);
  });
}

TEST_F(TimerTests, timerStartIsWorking) {
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    contextGroup.interrupt();
    ASSERT_ANY_THROW(t.sleep(std::chrono::milliseconds(1)));
    ASSERT_NO_THROW(t.sleep(std::chrono::milliseconds(1)));
  });
}

TEST_F(TimerTests, timerStopBeforeSleep) {
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    contextGroup.interrupt();
    ASSERT_THROW(t.sleep(std::chrono::milliseconds(1)), InterruptedException);
    contextGroup.interrupt();
    ASSERT_THROW(t.sleep(std::chrono::milliseconds(1)), InterruptedException);
  });
}

TEST_F(TimerTests, timerIsCancelable) {
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    ASSERT_THROW(t.sleep(std::chrono::milliseconds(100)), InterruptedException);
  });

  contextGroup.spawn([&]() { 
    contextGroup.interrupt(); 
  });
}

// Disabled, because on OS X it is currently impossible to distinguish timer timeout and interrupt
TEST_F(TimerTests, DISABLED_sleepThrowsOnlyIfTimerIsStoppedBeforeTime1) {
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    ASSERT_NO_THROW(t.sleep(std::chrono::milliseconds(1)));
    ASSERT_THROW(t.sleep(std::chrono::milliseconds(1)), InterruptedException);
  });

  contextGroup.spawn([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });
}

TEST_F(TimerTests, sleepIsSleepingAtLeastTakenTime) {
  auto timepoint1 = std::chrono::high_resolution_clock::now();
  contextGroup.spawn([&] {
    Timer(dispatcher).sleep(std::chrono::milliseconds(100));
  });

  contextGroup.wait();
  auto timepoint2 = std::chrono::high_resolution_clock::now();  
  ASSERT_LE(95, std::chrono::duration_cast<std::chrono::milliseconds>(timepoint2 - timepoint1).count());
}

TEST_F(TimerTests, timerIsReusable) {
  Timer t(dispatcher);
  auto timepoint1 = std::chrono::high_resolution_clock::now();
  contextGroup.spawn([&] {
    ASSERT_NO_THROW(t.sleep(std::chrono::seconds(1))); 
  });

  contextGroup.wait();
  auto timepoint2 = std::chrono::high_resolution_clock::now();
  contextGroup.spawn([&] {
    ASSERT_NO_THROW(t.sleep(std::chrono::seconds(1))); 
  });

  contextGroup.wait();
  auto timepoint3 = std::chrono::high_resolution_clock::now();
  ASSERT_LE(950, std::chrono::duration_cast<std::chrono::milliseconds>(timepoint2 - timepoint1).count());
  ASSERT_LE(950, std::chrono::duration_cast<std::chrono::milliseconds>(timepoint3 - timepoint2).count());
}

TEST_F(TimerTests, timerIsReusableAfterInterrupt) {
  contextGroup.spawn([&] {
    Timer t(dispatcher);
    contextGroup.interrupt();
    auto timepoint1 = std::chrono::high_resolution_clock::now();
    ASSERT_THROW(t.sleep(std::chrono::seconds(1)), InterruptedException);
    auto timepoint2 = std::chrono::high_resolution_clock::now();
    ASSERT_NO_THROW(t.sleep(std::chrono::seconds(1)));
    auto timepoint3 = std::chrono::high_resolution_clock::now();
    ASSERT_LE(0, std::chrono::duration_cast<std::chrono::milliseconds>(timepoint2 - timepoint1).count());
    ASSERT_LE(950, std::chrono::duration_cast<std::chrono::milliseconds>(timepoint3 - timepoint2).count());
  });
}

TEST_F(TimerTests, timerWithZeroTimeIsYielding) {
  bool done = false;
  contextGroup.spawn([&]() {
    done = true;
  });

  ASSERT_FALSE(done);
  Timer(dispatcher).sleep(std::chrono::milliseconds(0));
  ASSERT_TRUE(done);
}
