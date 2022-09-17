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

#include "CryptoNoteCore/CryptoNoteBasicImpl.h"
#include "CryptoNoteCore/Account.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/Currency.h"

#include <Common/Math.h>

#include "Chaingen.h"

using namespace CryptoNote;

bool test_transaction_generation_and_ring_signature()
{
  Logging::ConsoleLogger logger;
  CryptoNote::Currency currency = CryptoNote::CurrencyBuilder(logger).currency();

  AccountBase miner_acc1;
  miner_acc1.generate();
  AccountBase miner_acc2;
  miner_acc2.generate();
  AccountBase miner_acc3;
  miner_acc3.generate();
  AccountBase miner_acc4;
  miner_acc4.generate();
  AccountBase miner_acc5;
  miner_acc5.generate();
  AccountBase miner_acc6;
  miner_acc6.generate();

  std::string add_str = currency.accountAddressAsString(miner_acc3);

  AccountBase rv_acc;
  rv_acc.generate();
  AccountBase rv_acc2;
  rv_acc2.generate();
  Transaction tx_mine_1;
  currency.constructMinerTx(0, 0, 0, 10, 0, miner_acc1.getAccountKeys().address, tx_mine_1);
  Transaction tx_mine_2;
  currency.constructMinerTx(0, 0, 0, 0, 0, miner_acc2.getAccountKeys().address, tx_mine_2);
  Transaction tx_mine_3;
  currency.constructMinerTx(0, 0, 0, 0, 0, miner_acc3.getAccountKeys().address, tx_mine_3);
  Transaction tx_mine_4;
  currency.constructMinerTx(0, 0, 0, 0, 0, miner_acc4.getAccountKeys().address, tx_mine_4);
  Transaction tx_mine_5;
  currency.constructMinerTx(0, 0, 0, 0, 0, miner_acc5.getAccountKeys().address, tx_mine_5);
  Transaction tx_mine_6;
  currency.constructMinerTx(0, 0, 0, 0, 0, miner_acc6.getAccountKeys().address, tx_mine_6);

  //fill inputs entry
  typedef TransactionSourceEntry::OutputEntry tx_output_entry;
  std::vector<TransactionSourceEntry> sources;
  sources.resize(sources.size()+1);
  TransactionSourceEntry& src = sources.back();
  src.amount = 70368744177663;
  {
    tx_output_entry oe;
    oe.first = 0;
    oe.second = boost::get<KeyOutput>(tx_mine_1.outputs[0].target).key;
    src.outputs.push_back(oe);

    oe.first = 1;
    oe.second = boost::get<KeyOutput>(tx_mine_2.outputs[0].target).key;
    src.outputs.push_back(oe);

    oe.first = 2;
    oe.second = boost::get<KeyOutput>(tx_mine_3.outputs[0].target).key;
    src.outputs.push_back(oe);

    oe.first = 3;
    oe.second = boost::get<KeyOutput>(tx_mine_4.outputs[0].target).key;
    src.outputs.push_back(oe);

    oe.first = 4;
    oe.second = boost::get<KeyOutput>(tx_mine_5.outputs[0].target).key;
    src.outputs.push_back(oe);

    oe.first = 5;
    oe.second = boost::get<KeyOutput>(tx_mine_6.outputs[0].target).key;
    src.outputs.push_back(oe);

    src.realTransactionPublicKey = CryptoNote::getTransactionPublicKeyFromExtra(tx_mine_2.extra);
    src.realOutput = 1;
    src.realOutputIndexInTransaction = 0;
  }
  //fill outputs entry
  TransactionDestinationEntry td;
  td.addr = rv_acc.getAccountKeys().address;
  td.amount = 69368744177663;
  std::vector<TransactionDestinationEntry> destinations;
  destinations.push_back(td);

  Transaction tx_rc1;
  bool r = constructTransaction(miner_acc2.getAccountKeys(), sources, destinations, std::vector<uint8_t>(), tx_rc1, 0, logger);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");

  Crypto::Hash pref_hash = getObjectHash(*static_cast<TransactionPrefix*>(&tx_rc1));
  std::vector<const Crypto::PublicKey *> output_keys;
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_1.outputs[0].target).key);
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_2.outputs[0].target).key);
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_3.outputs[0].target).key);
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_4.outputs[0].target).key);
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_5.outputs[0].target).key);
  output_keys.push_back(&boost::get<KeyOutput>(tx_mine_6.outputs[0].target).key);
  r = Crypto::check_ring_signature(pref_hash, boost::get<KeyInput>(tx_rc1.inputs[0]).keyImage,
    output_keys, &tx_rc1.signatures[0][0]);
  CHECK_AND_ASSERT_MES(r, false, "failed to check ring signature");

  std::vector<size_t> outs;
  uint64_t money = 0;

  r = lookup_acc_outs(rv_acc.getAccountKeys(), tx_rc1, getTransactionPublicKeyFromExtra(tx_rc1.extra), outs, money);
  CHECK_AND_ASSERT_MES(r, false, "failed to lookup_acc_outs");
  CHECK_AND_ASSERT_MES(td.amount == money, false, "wrong money amount in new transaction");
  money = 0;
  r = lookup_acc_outs(rv_acc2.getAccountKeys(), tx_rc1, getTransactionPublicKeyFromExtra(tx_rc1.extra), outs,  money);
  CHECK_AND_ASSERT_MES(r, false, "failed to lookup_acc_outs");
  CHECK_AND_ASSERT_MES(0 == money, false, "wrong money amount in new transaction");
  return true;
}

bool test_block_creation()
{
  Logging::ConsoleLogger logger;

  uint64_t vszs[] = {80,476,476,475,475,474,475,474,474,475,472,476,476,475,475,474,475,474,474,475,472,476,476,475,475,474,475,474,474,475,9391,476,476,475,475,474,475,8819,8301,475,472,4302,5316,14347,16620,19583,19403,19728,19442,19852,19015,19000,19016,19795,19749,18087,19787,19704,19750,19267,19006,19050,19445,19407,19522,19546,19788,19369,19486,19329,19370,18853,19600,19110,19320,19746,19474,19474,19743,19494,19755,19715,19769,19620,19368,19839,19532,23424,28287,30707};
  std::vector<uint64_t> szs(&vszs[0], &vszs[90]);
  CryptoNote::Currency currency = CryptoNote::CurrencyBuilder(logger).currency();

  AccountPublicAddress adr;
  bool r = currency.parseAccountAddressString("272xWzbWsP4cfNFfxY5ETN5moU8x81PKfWPwynrrqsNGDBQGLmD1kCkKCvPeDUXu5XfmZkCrQ53wsWmdfvHBGLNjGcRiDcK", adr);
  CHECK_AND_ASSERT_MES(r, false, "failed to import");
  Block b;
  r = currency.constructMinerTx(90, Common::medianValue(szs), 3553616528562147, 33094, 10000000, adr, b.baseTransaction, BinaryArray(), 11);
  return r;
}

bool test_transactions()
{
  if(!test_transaction_generation_and_ring_signature())
    return false;
  if(!test_block_creation())
    return false;


  return true;
}
