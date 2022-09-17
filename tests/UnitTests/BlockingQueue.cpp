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
#include "Common/BlockingQueue.h"

#include <future>
#include <vector>
#include <numeric>
#include <memory>

class ParallelProcessor {
public:

  ParallelProcessor(size_t threads)
    : m_threads(threads) {}

  template <typename F>
  void spawn(F f) {
    for (auto& t : m_threads) {
      t = std::thread(f);
    }
  }

  void join() {
    for (auto& t : m_threads) {
      t.join();
    }
  }

private:

  std::vector<std::thread> m_threads;
  
};

// single producer, many consumers
void TestQueue_SPMC(unsigned iterations, unsigned threadCount, unsigned queueSize) {

  BlockingQueue<int> bq(queueSize);

  ParallelProcessor processor(threadCount);
  std::atomic<int64_t> result(0);

  processor.spawn([&bq, &result]{
      int v = 0;
      int64_t sum = 0;

      while (bq.pop(v)) {
        sum += v;
      }
      
      result += sum;
      // std::cout << "Sum: " << sum << std::endl;
    });

  int64_t expectedSum = 0;

  for (unsigned i = 0; i < iterations; ++i) {
    expectedSum += i;
    ASSERT_TRUE(bq.push(i));
  }

  bq.close();
  processor.join();

  ASSERT_EQ(expectedSum, result.load());
}

void TestQueue_MPSC(unsigned iterations, unsigned threadCount, unsigned queueSize) {

  BlockingQueue<int> bq(queueSize);

  ParallelProcessor processor(threadCount);
  std::atomic<unsigned> counter(0);
  std::atomic<int64_t> pushed(0);

  processor.spawn([&]{
    int64_t sum = 0;

    for(;;) {
      unsigned value = counter.fetch_add(1);
      if (value >= iterations)
        break;

      bq.push(value);
      sum += value;
    }

    pushed += sum;
    // std::cout << "Sum: " << sum << std::endl;
  });

  int64_t expectedSum = 0;

  for (unsigned i = 0; i < iterations; ++i) {
    int value;
    ASSERT_TRUE(bq.pop(value));
    expectedSum += i;
  }

  ASSERT_EQ(0, bq.size());

  processor.join();

  ASSERT_EQ(expectedSum, pushed);
}


TEST(BlockingQueue, SPMC)
{
  TestQueue_SPMC(10000, 1, 1);
  TestQueue_SPMC(10000, 4, 1);
  TestQueue_SPMC(10000, 16, 16);
  TestQueue_SPMC(10000, 16, 100);
}

TEST(BlockingQueue, MPSC)
{
  TestQueue_MPSC(10000, 1, 1);
  TestQueue_MPSC(10000, 4, 1);
  TestQueue_MPSC(10000, 16, 16);
  TestQueue_MPSC(10000, 16, 100);
}


TEST(BlockingQueue, PerfTest)
{
  // TestQueue_SPMC(1000000, 32, 1);
}

TEST(BlockingQueue, Close)
{
  BlockingQueue<int> bq(4);
  ParallelProcessor p(4);

  p.spawn([&bq] {
    int v;
    while (bq.pop(v))
      ;
  });

  bq.push(10); // enqueue 1 item

  bq.close(); // all threads should unblock and finish
  p.join(); 
}

TEST(BlockingQueue, CloseAndWait)
{
  size_t queueSize = 100;
  BlockingQueue<int> bq(queueSize);
  ParallelProcessor p(4);

  std::atomic<size_t> itemsPopped(0);

  // fill the queue
  for (int i = 0; i < queueSize; ++i)
    bq.push(i); 

  p.spawn([&bq, &itemsPopped] {
    int v;
    while (bq.pop(v)) {
      itemsPopped += 1;
      // some delay to make close() really wait
      std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }
  });


  // check with multiple closing
  auto f1 = std::async(std::launch::async, [&] { bq.close(true); });
  auto f2 = std::async(std::launch::async, [&] { bq.close(true); });

  bq.close(true);

  f1.get();
  f2.get();

  p.join();

  ASSERT_EQ(queueSize, itemsPopped.load());
}

TEST(BlockingQueue, AllowsMoveOnly)
{
  BlockingQueue<std::unique_ptr<int>> bq(1);

  std::unique_ptr<int> v(new int(100));
  ASSERT_TRUE(bq.push(std::move(v)));

  std::unique_ptr<int> popval;
  bq.pop(popval);

  ASSERT_EQ(*popval, 100);
}
