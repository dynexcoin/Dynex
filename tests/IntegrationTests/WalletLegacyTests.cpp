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

#include "BaseTests.h"

#include <System/Timer.h>
#include "WalletLegacy/WalletLegacy.h"
#include "WalletLegacyObserver.h"

using namespace Tests;
using namespace CryptoNote;

class WalletLegacyTests : public BaseTest {

};


TEST_F(WalletLegacyTests, checkNetworkShutdown) {
  auto networkCfg = TestNetworkBuilder(3, Topology::Star).
    setBlockchain("testnet_300").build();

  networkCfg[0].nodeType = NodeType::InProcess;
  network.addNodes(networkCfg);
  network.waitNodesReady();

  auto& daemon = network.getNode(0);

  {
    auto node = daemon.makeINode();
    WalletLegacy wallet(currency, *node);
    wallet.initAndGenerate("pass");

    WalletLegacyObserver observer;
    wallet.addObserver(&observer);

    std::error_code syncResult;
    ASSERT_TRUE(observer.m_syncResult.waitFor(std::chrono::seconds(10), syncResult));
    ASSERT_TRUE(!syncResult);

    // sync completed
    auto syncProgress = observer.getSyncProgress();

    network.getNode(1).stopDaemon();
    network.getNode(2).stopDaemon();

    System::Timer(dispatcher).sleep(std::chrono::seconds(10));

    // check that sync progress was not updated
    ASSERT_EQ(syncProgress, observer.getSyncProgress()); 
  }
}
