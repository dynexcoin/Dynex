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

#pragma once 

#include "Chaingen.h"

class CheckBlockPurged : public test_chain_unit_base {
public:
  CheckBlockPurged(size_t invalidBlockIdx) :
    m_invalidBlockIdx(invalidBlockIdx) {

    CryptoNote::CurrencyBuilder currencyBuilder(m_logger);
    m_currency = currencyBuilder.currency();

    REGISTER_CALLBACK("check_block_purged", CheckBlockPurged::check_block_purged);
    REGISTER_CALLBACK("markInvalidBlock", CheckBlockPurged::markInvalidBlock);
  }

  bool check_block_verification_context(const CryptoNote::block_verification_context& bvc, size_t eventIdx, const CryptoNote::Block& /*blk*/) {
    if (m_invalidBlockIdx == eventIdx) {
      return bvc.m_verifivation_failed;
    } else {
      return !bvc.m_verifivation_failed;
    }
  }

  bool check_block_purged(CryptoNote::core& c, size_t eventIdx, const std::vector<test_event_entry>& events) {
    DEFINE_TESTS_ERROR_CONTEXT("CheckBlockPurged::check_block_purged");

    CHECK_TEST_CONDITION(m_invalidBlockIdx < eventIdx);
    CHECK_EQ(0, c.get_pool_transactions_count());
    CHECK_EQ(m_invalidBlockIdx, c.get_current_blockchain_height());

    return true;
  }

  bool markInvalidBlock(CryptoNote::core& c, size_t eventIdx, const std::vector<test_event_entry>& events) {
    m_invalidBlockIdx = eventIdx + 1;
    return true;
  }

protected:
  size_t m_invalidBlockIdx;
};


struct CheckBlockAccepted : public test_chain_unit_base {
  CheckBlockAccepted(size_t expectedBlockchainHeight) :
    m_expectedBlockchainHeight(expectedBlockchainHeight) {

    CryptoNote::CurrencyBuilder currencyBuilder(m_logger);
    m_currency = currencyBuilder.currency();

    REGISTER_CALLBACK("check_block_accepted", CheckBlockAccepted::check_block_accepted);
  }

  bool check_block_accepted(CryptoNote::core& c, size_t /*eventIdx*/, const std::vector<test_event_entry>& /*events*/) {
    DEFINE_TESTS_ERROR_CONTEXT("CheckBlockAccepted::check_block_accepted");

    CHECK_EQ(0, c.get_pool_transactions_count());
    CHECK_EQ(m_expectedBlockchainHeight, c.get_current_blockchain_height());

    return true;
  }

protected:
  size_t m_expectedBlockchainHeight;
};


struct TestBlockMajorVersionAccepted : public CheckBlockAccepted {
  TestBlockMajorVersionAccepted() :
    CheckBlockAccepted(2) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct TestBlockMajorVersionRejected : public CheckBlockPurged {
  TestBlockMajorVersionRejected() :
    CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct TestBlockBigMinorVersion : public CheckBlockAccepted {

  TestBlockBigMinorVersion()
    : CheckBlockAccepted(2) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_not_checked : public CheckBlockAccepted
{
  gen_block_ts_not_checked()
    : CheckBlockAccepted(0) {
    m_expectedBlockchainHeight = m_currency.timestampCheckWindow();
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_in_past : public CheckBlockPurged
{
  gen_block_ts_in_past()
    : CheckBlockPurged(0) {
    m_invalidBlockIdx = m_currency.timestampCheckWindow();
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_in_future_rejected : public CheckBlockPurged
{
  gen_block_ts_in_future_rejected()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_in_future_accepted : public CheckBlockAccepted
{
  gen_block_ts_in_future_accepted()
    : CheckBlockAccepted(2) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_invalid_prev_id : public CheckBlockPurged
{
  gen_block_invalid_prev_id() 
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
  bool check_block_verification_context(const CryptoNote::block_verification_context& bvc, size_t event_idx, const CryptoNote::Block& /*blk*/);
};

struct gen_block_invalid_nonce : public CheckBlockPurged
{
  gen_block_invalid_nonce()
    : CheckBlockPurged(3) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_no_miner_tx : public CheckBlockPurged
{
  gen_block_no_miner_tx()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_low : public CheckBlockPurged
{
  gen_block_unlock_time_is_low()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_high : public CheckBlockPurged
{
  gen_block_unlock_time_is_high()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_timestamp_in_past : public CheckBlockPurged
{
  gen_block_unlock_time_is_timestamp_in_past()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_timestamp_in_future : public CheckBlockPurged
{
  gen_block_unlock_time_is_timestamp_in_future()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_height_is_low : public CheckBlockPurged
{
  gen_block_height_is_low()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_height_is_high : public CheckBlockPurged
{
  gen_block_height_is_high()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_2_tx_gen_in : public CheckBlockPurged
{
  gen_block_miner_tx_has_2_tx_gen_in()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_2_in : public CheckBlockPurged
{
  gen_block_miner_tx_has_2_in()
    : CheckBlockPurged(0) {
    m_invalidBlockIdx = m_currency.minedMoneyUnlockWindow() + 1;
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_with_txin_to_key : public CheckBlockPurged
{
  gen_block_miner_tx_with_txin_to_key()
    : CheckBlockPurged(0) {
    m_invalidBlockIdx = m_currency.minedMoneyUnlockWindow() + 2;
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_out_is_small : public CheckBlockPurged
{
  gen_block_miner_tx_out_is_small()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_out_is_big : public CheckBlockPurged
{
  gen_block_miner_tx_out_is_big()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_no_out : public CheckBlockPurged
{
  gen_block_miner_tx_has_no_out()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_out_to_alice : public CheckBlockAccepted
{
  gen_block_miner_tx_has_out_to_alice()
    : CheckBlockAccepted(2) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_has_invalid_tx : public CheckBlockPurged
{
  gen_block_has_invalid_tx()
    : CheckBlockPurged(1) {}

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_is_too_big : public CheckBlockPurged
{
  gen_block_is_too_big()
      : CheckBlockPurged(1) {
    CryptoNote::CurrencyBuilder currencyBuilder(m_logger);
    currencyBuilder.maxBlockSizeInitial(std::numeric_limits<size_t>::max() / 2);
    m_currency = currencyBuilder.currency();
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct TestBlockCumulativeSizeExceedsLimit : public CheckBlockPurged {
  TestBlockCumulativeSizeExceedsLimit()
    : CheckBlockPurged(std::numeric_limits<size_t>::max()) {
  }

  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_invalid_binary_format : public test_chain_unit_base
{
  gen_block_invalid_binary_format();

  bool generate(std::vector<test_event_entry>& events) const;
  bool check_block_verification_context(const CryptoNote::block_verification_context& bvc, size_t event_idx, const CryptoNote::Block& /*blk*/);
  bool check_all_blocks_purged(CryptoNote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool corrupt_blocks_boundary(CryptoNote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  size_t m_corrupt_blocks_begin_idx;
};
