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

#include "Logging/LoggerManager.h"
#include "System/Dispatcher.h"
#include "System/InterruptedException.h"

#include "../IntegrationTestLib/BaseFunctionalTests.h"
#include "../IntegrationTestLib/TestWalletLegacy.h"


using namespace CryptoNote;
using namespace Crypto;
using namespace Tests::Common;

extern System::Dispatcher globalSystem;
extern Tests::Common::BaseFunctionalTestsConfig config;

namespace {
  class NodeRpcProxyTest : public Tests::Common::BaseFunctionalTests, public ::testing::Test {
  public:
    NodeRpcProxyTest() :
      BaseFunctionalTests(m_currency, globalSystem, config),
      m_currency(CurrencyBuilder(m_logManager).testnet(true).currency()) {
    }

  protected:
    Logging::LoggerManager m_logManager;
    CryptoNote::Currency m_currency;
  };

  class PoolChangedObserver : public INodeObserver {
  public:
    virtual void poolChanged() override {
      std::unique_lock<std::mutex> lk(mutex);
      ready = true;
      cv.notify_all();
    }

    bool waitPoolChanged(size_t seconds) {
      std::unique_lock<std::mutex> lk(mutex);
      bool r = cv.wait_for(lk, std::chrono::seconds(seconds), [this]() { return ready; });
      ready = false;
      return r;
    }

  private:
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
  };

  TEST_F(NodeRpcProxyTest, PoolChangedCalledWhenTxCame) {
    const size_t NODE_0 = 0;
    const size_t NODE_1 = 1;

    launchTestnet(2, Tests::Common::BaseFunctionalTests::Line);

    std::unique_ptr<CryptoNote::INode> node0;
    std::unique_ptr<CryptoNote::INode> node1;

    nodeDaemons[NODE_0]->makeINode(node0);
    nodeDaemons[NODE_1]->makeINode(node1);

    TestWalletLegacy wallet1(m_dispatcher, m_currency, *node0);
    TestWalletLegacy wallet2(m_dispatcher, m_currency, *node1);

    ASSERT_FALSE(static_cast<bool>(wallet1.init()));
    ASSERT_FALSE(static_cast<bool>(wallet2.init()));

    ASSERT_TRUE(mineBlocks(*nodeDaemons[NODE_0], wallet1.address(), 1));
    ASSERT_TRUE(mineBlocks(*nodeDaemons[NODE_0], wallet1.address(), m_currency.minedMoneyUnlockWindow()));

    wallet1.waitForSynchronizationToHeight(static_cast<uint32_t>(m_currency.minedMoneyUnlockWindow()) + 1);
    wallet2.waitForSynchronizationToHeight(static_cast<uint32_t>(m_currency.minedMoneyUnlockWindow()) + 1);

    PoolChangedObserver observer;
    node0->addObserver(&observer);

    Hash dontCare;
    ASSERT_FALSE(static_cast<bool>(wallet1.sendTransaction(m_currency.accountAddressAsString(wallet2.address()), m_currency.coin(), dontCare)));
    ASSERT_TRUE(observer.waitPoolChanged(10));
  }
}
