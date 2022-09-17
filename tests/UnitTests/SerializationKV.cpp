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

#include "gtest/gtest.h"

#include <boost/lexical_cast.hpp>

#include "Serialization/KVBinaryInputStreamSerializer.h"
#include "Serialization/KVBinaryOutputStreamSerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "Serialization/SerializationTools.h"

#include <array>

using namespace CryptoNote;

namespace CryptoNote {

struct TestElement {
  std::string name;
  uint32_t nonce;
  std::array<uint8_t, 16> blob;
  std::vector<uint32_t> u32array;

  bool operator == (const TestElement& other) const {
    return 
      name == other.name && 
      nonce == other.nonce &&
      blob == other.blob && 
      u32array == other.u32array;
  }

  void serialize(ISerializer& s) {
    s(name, "name");
    s(nonce, "nonce");
    s.binary(blob.data(), blob.size(), "blob");
    serializeAsBinary(u32array, "u32array", s);
  }
};

struct TestStruct {
  uint8_t u8;
  uint32_t u32;
  uint64_t u64;
  std::vector<TestElement> vec1;
  std::vector<TestElement> vec2;
  TestElement root;

  bool operator == (const TestStruct& other) const {
    return
      root == other.root &&
      u8 == other.u8 &&
      u32 == other.u32 &&
      u64 == other.u64 &&
      vec1 == other.vec1 &&
      vec2 == other.vec2;
  }

  void serialize(ISerializer& s) {
    s(root, "root");
    s(vec1, "vec1");
    s(vec2, "vec2");
    s(u8, "u8");
    s(u32, "u32");
    s(u64, "u64");
  }

};

}


#include <chrono>

typedef std::chrono::high_resolution_clock hclock;

class HiResTimer {
public:
  HiResTimer() : 
    start(hclock::now()) {}

  std::chrono::duration<double> duration() {
    return hclock::now() - start;
  }

private:
  hclock::time_point start;
};

TEST(KVSerialize, Simple) {
  TestElement testData1, testData2;

  testData1.name = "hello";
  testData1.nonce = 12345;
  testData1.u32array.resize(128);

  testData2.name = "bye";
  testData2.nonce = 54321;

  std::string buf = CryptoNote::storeToBinaryKeyValue(testData1);
  ASSERT_TRUE(CryptoNote::loadFromBinaryKeyValue(testData2, buf));
  EXPECT_EQ(testData1, testData2);
}

TEST(KVSerialize, BigCollection) {
  TestStruct ts1;

  ts1.u8 = 100;
  ts1.u32 = 0xff0000;
  ts1.u64 = 1ULL << 60;
  ts1.root.name = "hello";

  TestElement sample;
  sample.nonce = 101;
  ts1.vec1.resize(0x10000 >> 2, sample);

  TestStruct ts2;

  std::string buf = CryptoNote::storeToBinaryKeyValue(ts1);
  ASSERT_TRUE(CryptoNote::loadFromBinaryKeyValue(ts2, buf));
  EXPECT_EQ(ts1, ts2);
}
