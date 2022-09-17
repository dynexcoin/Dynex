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

#include <System/Dispatcher.h>
#include <System/ContextGroup.h>
#include <System/InterruptedException.h>
#include <System/Ipv4Address.h>
#include <System/Ipv4Resolver.h>
#include <gtest/gtest.h>

using namespace System;

class Ipv4ResolverTests : public testing::Test {
public:
  Ipv4ResolverTests() : contextGroup(dispatcher), resolver(dispatcher) {
  }

  Dispatcher dispatcher;
  ContextGroup contextGroup;
  Ipv4Resolver resolver;
};

TEST_F(Ipv4ResolverTests, start) {
  contextGroup.spawn([&] { 
    ASSERT_NO_THROW(Ipv4Resolver(dispatcher).resolve("localhost")); 
  });
  contextGroup.wait();
}

TEST_F(Ipv4ResolverTests, stop) {
  contextGroup.spawn([&] {
    contextGroup.interrupt();
    ASSERT_THROW(resolver.resolve("localhost"), InterruptedException);
  });
  contextGroup.wait();
}

TEST_F(Ipv4ResolverTests, interruptWhileResolving) {
  contextGroup.spawn([&] {
    ASSERT_THROW(resolver.resolve("localhost"), InterruptedException);
  });
  contextGroup.interrupt();
  contextGroup.wait();
}

TEST_F(Ipv4ResolverTests, reuseAfterInterrupt) {
  contextGroup.spawn([&] {
    ASSERT_THROW(resolver.resolve("localhost"), InterruptedException);
  });
  contextGroup.interrupt();
  contextGroup.wait();
  contextGroup.spawn([&] {
    ASSERT_NO_THROW(resolver.resolve("localhost"));
  });
  contextGroup.wait();
}

TEST_F(Ipv4ResolverTests, resolve) {
  ASSERT_EQ(Ipv4Address("0.0.0.0"), resolver.resolve("0.0.0.0"));
  ASSERT_EQ(Ipv4Address("1.2.3.4"), resolver.resolve("1.2.3.4"));
  ASSERT_EQ(Ipv4Address("127.0.0.1"), resolver.resolve("127.0.0.1"));
  ASSERT_EQ(Ipv4Address("254.253.252.251"), resolver.resolve("254.253.252.251"));
  ASSERT_EQ(Ipv4Address("255.255.255.255"), resolver.resolve("255.255.255.255"));
  ASSERT_EQ(Ipv4Address("127.0.0.1"), resolver.resolve("localhost"));
//ASSERT_EQ(Ipv4Address("93.184.216.34"), resolver.resolve("example.com"));
  ASSERT_THROW(resolver.resolve(".0.0.0.0"), std::runtime_error);
  ASSERT_THROW(resolver.resolve("0..0.0.0"), std::runtime_error);
//ASSERT_THROW(resolver.resolve("0.0.0"), std::runtime_error);
  ASSERT_THROW(resolver.resolve("0.0.0."), std::runtime_error);
//ASSERT_THROW(resolver.resolve("0.0.0.0."), std::runtime_error);
  ASSERT_THROW(resolver.resolve("0.0.0.0.0"), std::runtime_error);
//ASSERT_THROW(resolver.resolve("0.0.0.00"), std::runtime_error);
//ASSERT_THROW(resolver.resolve("0.0.0.01"), std::runtime_error);
  ASSERT_THROW(resolver.resolve("0.0.0.256"), std::runtime_error);
//ASSERT_THROW(resolver.resolve("00.0.0.0"), std::runtime_error);
//ASSERT_THROW(resolver.resolve("01.0.0.0"), std::runtime_error);
  ASSERT_THROW(resolver.resolve("256.0.0.0"), std::runtime_error);
  ASSERT_THROW(resolver.resolve("invalid"), std::runtime_error);
}
