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

#include <gtest/gtest.h>
#include <System/ContextGroupTimeout.h>
#include <System/InterruptedException.h>

using namespace System;

class ContextGroupTimeoutTest : public testing::Test {
public:
  ContextGroupTimeoutTest() : contextGroup(dispatcher), timer(dispatcher) {
  }

  Dispatcher dispatcher;
  ContextGroup contextGroup;
  Timer timer;
};

TEST_F(ContextGroupTimeoutTest, timeoutHappens) {
  auto begin = std::chrono::high_resolution_clock::now();
  ContextGroupTimeout groupTimeout(dispatcher, contextGroup, std::chrono::milliseconds(100));
  contextGroup.spawn([&] {
    EXPECT_THROW(Timer(dispatcher).sleep(std::chrono::milliseconds(200)), InterruptedException);
  });
  contextGroup.wait();
  ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin).count(), 50);
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(150));
}

TEST_F(ContextGroupTimeoutTest, timeoutSkipped) {
  auto begin = std::chrono::high_resolution_clock::now();
  {
    ContextGroupTimeout op(dispatcher, contextGroup, std::chrono::milliseconds(200));
    contextGroup.spawn([&] {
      EXPECT_NO_THROW(Timer(dispatcher).sleep(std::chrono::milliseconds(100)));
    });
    contextGroup.wait();
  }
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) > std::chrono::milliseconds(50));
  ASSERT_TRUE((std::chrono::high_resolution_clock::now() - begin) < std::chrono::milliseconds(150));
}

TEST_F(ContextGroupTimeoutTest, noOperation) {
  ContextGroupTimeout op(dispatcher, contextGroup, std::chrono::milliseconds(100));
}
