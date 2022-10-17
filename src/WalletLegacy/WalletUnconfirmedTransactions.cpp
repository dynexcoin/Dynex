// Copyright (c) 2021-2022, Dynex Developers
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
// Parts of this project are originally copyright by:
// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#include "WalletUnconfirmedTransactions.h"
#include "WalletLegacy/WalletLegacySerialization.h"

#include "CryptoNoteCore/CryptoNoteTools.h"
#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"

using namespace Crypto;

namespace CryptoNote {

inline TransactionOutputId getOutputId(const TransactionOutputInformation& out) {
  return std::make_pair(out.transactionPublicKey, out.outputInTransaction);
}

WalletUnconfirmedTransactions::WalletUnconfirmedTransactions(uint64_t uncofirmedTransactionsLiveTime):
  m_uncofirmedTransactionsLiveTime(uncofirmedTransactionsLiveTime) {

}

bool WalletUnconfirmedTransactions::serialize(ISerializer& s) {
  s(m_unconfirmedTxs, "transactions");
  if (s.type() == ISerializer::INPUT) {
    collectUsedOutputs();
  }
  return true;
}

bool WalletUnconfirmedTransactions::findTransactionId(const Hash& hash, TransactionId& id) {
  auto it = m_unconfirmedTxs.find(hash);
  if (it == m_unconfirmedTxs.end()) {
    return false;
  }

  id = it->second.transactionId;
  return true;
}

void WalletUnconfirmedTransactions::erase(const Hash& hash) {
  auto it = m_unconfirmedTxs.find(hash);
  if (it == m_unconfirmedTxs.end()) {
    return;
  }

  deleteUsedOutputs(it->second.usedOutputs);
  m_unconfirmedTxs.erase(it);
}

void WalletUnconfirmedTransactions::add(const Transaction& tx, TransactionId transactionId, 
  uint64_t amount, const std::list<TransactionOutputInformation>& usedOutputs, Crypto::SecretKey& tx_key) {

  UnconfirmedTransferDetails& utd = m_unconfirmedTxs[getObjectHash(tx)];

  utd.amount = amount;
  utd.sentTime = time(nullptr);
  utd.tx = tx;
  utd.transactionId = transactionId;
  utd.secretKey = tx_key;

  uint64_t outsAmount = 0;
  // process used outputs
  utd.usedOutputs.reserve(usedOutputs.size());
  for (const auto& out : usedOutputs) {
    auto id = getOutputId(out);
    utd.usedOutputs.push_back(id);
    m_usedOutputs.insert(id);
    outsAmount += out.amount;
  }

  utd.outsAmount = outsAmount;
}

void WalletUnconfirmedTransactions::updateTransactionId(const Hash& hash, TransactionId id) {
  auto it = m_unconfirmedTxs.find(hash);
  if (it != m_unconfirmedTxs.end()) {
    it->second.transactionId = id;
  }
}

uint64_t WalletUnconfirmedTransactions::countUnconfirmedOutsAmount() const {
  uint64_t amount = 0;

  for (auto& utx: m_unconfirmedTxs)
    amount+= utx.second.outsAmount;

  return amount;
}

uint64_t WalletUnconfirmedTransactions::countUnconfirmedTransactionsAmount() const {
  uint64_t amount = 0;

  for (auto& utx: m_unconfirmedTxs)
    amount+= utx.second.amount;

  return amount;
}

bool WalletUnconfirmedTransactions::isUsed(const TransactionOutputInformation& out) const {
  return m_usedOutputs.find(getOutputId(out)) != m_usedOutputs.end();
}

void WalletUnconfirmedTransactions::collectUsedOutputs() {
  UsedOutputsContainer used;
  for (const auto& kv : m_unconfirmedTxs) {
    used.insert(kv.second.usedOutputs.begin(), kv.second.usedOutputs.end());
  }
  m_usedOutputs = std::move(used);
}

void WalletUnconfirmedTransactions::reset() {
  m_unconfirmedTxs.clear();
  m_usedOutputs.clear();
}

void WalletUnconfirmedTransactions::deleteUsedOutputs(const std::vector<TransactionOutputId>& usedOutputs) {
  for (const auto& output: usedOutputs) {
    m_usedOutputs.erase(output);
  }
}

std::vector<TransactionId> WalletUnconfirmedTransactions::deleteOutdatedTransactions() {
  std::vector<TransactionId> deletedTransactions;

  uint64_t now = static_cast<uint64_t>(time(nullptr));
  assert(now >= m_uncofirmedTransactionsLiveTime);

  for (auto it = m_unconfirmedTxs.begin(); it != m_unconfirmedTxs.end();) {
    if (static_cast<uint64_t>(it->second.sentTime) <= now - m_uncofirmedTransactionsLiveTime) {
      deleteUsedOutputs(it->second.usedOutputs);
      deletedTransactions.push_back(it->second.transactionId);
      it = m_unconfirmedTxs.erase(it);
    } else {
      ++it;
    }
  }

  return deletedTransactions;
}

} /* namespace CryptoNote */
