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

#include "RandomOuts.h"
#include "TestGenerator.h"
#include "Rpc/CoreRpcServerCommandsDefinitions.h"

GetRandomOutputs::GetRandomOutputs() {
  REGISTER_CALLBACK_METHOD(GetRandomOutputs, checkHalfUnlocked);
  REGISTER_CALLBACK_METHOD(GetRandomOutputs, checkFullyUnlocked);
}

bool GetRandomOutputs::generate(std::vector<test_event_entry>& events) const {
  TestGenerator generator(m_currency, events);

  generator.generateBlocks();

  uint64_t sendAmount = MK_COINS(1);

  auto builder = generator.createTxBuilder(
    generator.minerAccount, generator.minerAccount, sendAmount, m_currency.minimumFee());

  for (int i = 0; i < 10; ++i) {
    auto builder = generator.createTxBuilder(
      generator.minerAccount, generator.minerAccount, sendAmount, m_currency.minimumFee());

    auto tx = builder.build();
    generator.addEvent(tx);
    generator.makeNextBlock(tx);
  }

  // unlock half of the money
  generator.generateBlocks(m_currency.minedMoneyUnlockWindow() / 2);
  generator.addCallback("checkHalfUnlocked");

  // unlock the remaining part
  generator.generateBlocks(m_currency.minedMoneyUnlockWindow() / 2);
  generator.addCallback("checkFullyUnlocked");
  
  return true;
}

bool GetRandomOutputs::request(CryptoNote::core& c, uint64_t amount, size_t mixin, CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& resp) {
  CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request req;

  req.amounts.push_back(amount);
  req.outs_count = mixin;

  resp = boost::value_initialized<CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response>();

  return c.get_random_outs_for_amounts(req, resp);
}

#define CHECK(cond) if((cond) == false) { LOG_ERROR("Condition "#cond" failed"); return false; }

bool GetRandomOutputs::checkHalfUnlocked(CryptoNote::core& c, size_t ev_index, const std::vector<test_event_entry>& events) {
  CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response resp;

  auto amount = MK_COINS(1);
  auto unlocked = m_currency.minedMoneyUnlockWindow() / 2 + 1;

  CHECK(request(c, amount, 0, resp));
  CHECK(resp.outs.size() == 1);
  CHECK(resp.outs[0].amount == amount);
  CHECK(resp.outs[0].outs.size() == 0);

  CHECK(request(c, amount, unlocked, resp));
  CHECK(resp.outs.size() == 1);
  CHECK(resp.outs[0].amount == amount);
  CHECK(resp.outs[0].outs.size() == unlocked);

  CHECK(request(c, amount, unlocked * 2, resp));
  CHECK(resp.outs.size() == 1);
  CHECK(resp.outs[0].amount == amount);
  CHECK(resp.outs[0].outs.size() == unlocked);

  return true;
}

bool GetRandomOutputs::checkFullyUnlocked(CryptoNote::core& c, size_t ev_index, const std::vector<test_event_entry>& events) {
  CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response resp;

  auto amount = MK_COINS(1);
  auto unlocked = m_currency.minedMoneyUnlockWindow() + 1;

  CHECK(request(c, amount, unlocked, resp));
  CHECK(resp.outs.size() == 1);
  CHECK(resp.outs[0].amount == amount);
  CHECK(resp.outs[0].outs.size() == unlocked);

  CHECK(request(c, amount, unlocked * 2, resp));
  CHECK(resp.outs.size() == 1);
  CHECK(resp.outs[0].amount == amount);
  CHECK(resp.outs[0].outs.size() == unlocked);

  return true;
}
