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

#include "IWalletLegacy.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "../IntegrationTestLib/ObservableValue.h"

namespace CryptoNote {

class WalletLegacyObserver: public IWalletLegacyObserver {
public:

  WalletLegacyObserver() :
    m_actualBalance(0),
    m_actualBalancePrev(0),
    m_pendingBalance(0),
    m_pendingBalancePrev(0),
    m_syncResult(m_mutex, m_cv) {}

  virtual void actualBalanceUpdated(uint64_t actualBalance) override {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_actualBalance = actualBalance;
    lk.unlock();
    m_cv.notify_all();
  }

  virtual void pendingBalanceUpdated(uint64_t pendingBalance) override {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_pendingBalance = pendingBalance;
    lk.unlock();
    m_cv.notify_all();
  }

  virtual void sendTransactionCompleted(CryptoNote::TransactionId transactionId, std::error_code result) override {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_sendResults[transactionId] = result;
    m_cv.notify_all();
  }

  virtual void synchronizationCompleted(std::error_code result) override {
    m_syncResult.set(result);
  }

  virtual void synchronizationProgressUpdated(uint32_t current, uint32_t total) override {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_syncProgress.emplace_back(current, total);
    m_currentHeight = current;
    m_cv.notify_all();
  }

  virtual void externalTransactionCreated(TransactionId transactionId) override {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_externalTransactions.push_back(transactionId);
    m_cv.notify_all();
  }
  
  uint64_t getCurrentHeight() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_currentHeight;
  }

  uint64_t waitPendingBalanceChange() {
    std::unique_lock<std::mutex> lk(m_mutex);
    while (m_pendingBalance == m_pendingBalancePrev) {
      m_cv.wait(lk);
    }
    m_pendingBalancePrev = m_pendingBalance;
    return m_pendingBalance;
  }

  uint64_t waitTotalBalanceChange() {
    std::unique_lock<std::mutex> lk(m_mutex);
    while (m_pendingBalance == m_pendingBalancePrev && m_actualBalance == m_actualBalancePrev) {
      m_cv.wait(lk);
    }

    m_actualBalancePrev = m_actualBalance;
    m_pendingBalancePrev = m_pendingBalance;

    return m_actualBalance + m_pendingBalance;
  }

  CryptoNote::TransactionId waitExternalTransaction() {
    std::unique_lock<std::mutex> lk(m_mutex);

    while (m_externalTransactions.empty()) {
      m_cv.wait(lk);
    }

    CryptoNote::TransactionId txId = m_externalTransactions.front();
    m_externalTransactions.pop_front();
    return txId;
  }

  template<class Rep, class Period>
  std::pair<bool, uint64_t> waitPendingBalanceChangeFor(const std::chrono::duration<Rep, Period>& timePeriod) {
    std::unique_lock<std::mutex> lk(m_mutex);
    bool result = m_cv.wait_for(lk, timePeriod, [&] { return m_pendingBalance != m_pendingBalancePrev; });
    m_pendingBalancePrev = m_pendingBalance;
    return std::make_pair(result, m_pendingBalance);
  }

  uint64_t waitActualBalanceChange() {
    std::unique_lock<std::mutex> lk(m_mutex);
    while (m_actualBalance == m_actualBalancePrev) {
      m_cv.wait(lk);
    }
    m_actualBalancePrev = m_actualBalance;
    return m_actualBalance;
  }

  std::error_code waitSendResult(CryptoNote::TransactionId txid) {
    std::unique_lock<std::mutex> lk(m_mutex);

    std::unordered_map<CryptoNote::TransactionId, std::error_code>::iterator it;

    while ((it = m_sendResults.find(txid)) == m_sendResults.end()) {
      m_cv.wait(lk);
    }

    return it->second;
  }

  uint64_t totalBalance() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_pendingBalancePrev = m_pendingBalance;
    m_actualBalancePrev = m_actualBalance;
    return m_pendingBalance + m_actualBalance;
  }

  std::vector<std::pair<uint32_t, uint32_t>> getSyncProgress() {
    std::unique_lock<std::mutex> lk(m_mutex);
    return m_syncProgress;
  }

  ObservableValueBase<std::error_code> m_syncResult;

private:
    
  std::mutex m_mutex;
  std::condition_variable m_cv;

  uint64_t m_actualBalance;
  uint64_t m_actualBalancePrev;
  uint64_t m_pendingBalance;
  uint64_t m_pendingBalancePrev;

  uint32_t m_currentHeight;

  std::vector<std::pair<uint32_t, uint32_t>> m_syncProgress;

  std::unordered_map<CryptoNote::TransactionId, std::error_code> m_sendResults;
  std::deque<CryptoNote::TransactionId> m_externalTransactions;
};

}
