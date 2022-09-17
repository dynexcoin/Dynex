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

#include "IWallet.h"
#include "WalletIndices.h"
#include "Common/IInputStream.h"
#include "Common/IOutputStream.h"
#include "Transfers/TransfersSynchronizer.h"
#include "Serialization/BinaryInputStreamSerializer.h"

#include "crypto/chacha8.h"

namespace CryptoNote {

struct CryptoContext {
  Crypto::chacha8_key key;
  Crypto::chacha8_iv iv;

  void incIv();
};

class WalletSerializer {
public:
  WalletSerializer(
    ITransfersObserver& transfersObserver,
    Crypto::PublicKey& viewPublicKey,
    Crypto::SecretKey& viewSecretKey,
    uint64_t& actualBalance,
    uint64_t& pendingBalance,
    WalletsContainer& walletsContainer,
    TransfersSyncronizer& synchronizer,
    UnlockTransactionJobs& unlockTransactions,
    WalletTransactions& transactions,
    WalletTransfers& transfers,
    uint32_t transactionSoftLockTime,
    UncommitedTransactions& uncommitedTransactions
  );
  
  void save(const std::string& password, Common::IOutputStream& destination, bool saveDetails, bool saveCache);
  void load(const std::string& password, Common::IInputStream& source);

private:
  static const uint32_t SERIALIZATION_VERSION;

  void loadWallet(Common::IInputStream& source, const std::string& password, uint32_t version);
  void loadWalletV1(Common::IInputStream& source, const std::string& password);

  CryptoContext generateCryptoContext(const std::string& password);

  void saveVersion(Common::IOutputStream& destination);
  void saveIv(Common::IOutputStream& destination, Crypto::chacha8_iv& iv);
  void saveKeys(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void savePublicKey(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveSecretKey(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveFlags(bool saveDetails, bool saveCache, Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveWallets(Common::IOutputStream& destination, bool saveCache, CryptoContext& cryptoContext);
  void saveBalances(Common::IOutputStream& destination, bool saveCache, CryptoContext& cryptoContext);
  void saveTransfersSynchronizer(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveUnlockTransactionsJobs(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveUncommitedTransactions(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveTransactions(Common::IOutputStream& destination, CryptoContext& cryptoContext);
  void saveTransfers(Common::IOutputStream& destination, CryptoContext& cryptoContext);

  uint32_t loadVersion(Common::IInputStream& source);
  void loadIv(Common::IInputStream& source, Crypto::chacha8_iv& iv);
  void generateKey(const std::string& password, Crypto::chacha8_key& key);
  void loadKeys(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadPublicKey(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadSecretKey(Common::IInputStream& source, CryptoContext& cryptoContext);
  void checkKeys();
  void loadFlags(bool& details, bool& cache, Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadWallets(Common::IInputStream& source, CryptoContext& cryptoContext);
  void subscribeWallets();
  void loadBalances(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadTransfersSynchronizer(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadObsoleteSpentOutputs(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadUnlockTransactionsJobs(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadObsoleteChange(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadUncommitedTransactions(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadTransactions(Common::IInputStream& source, CryptoContext& cryptoContext);
  void loadTransfers(Common::IInputStream& source, CryptoContext& cryptoContext, uint32_t version);

  void loadWalletV1Keys(CryptoNote::BinaryInputStreamSerializer& serializer);
  void loadWalletV1Details(CryptoNote::BinaryInputStreamSerializer& serializer);
  void addWalletV1Details(const std::vector<WalletLegacyTransaction>& txs, const std::vector<WalletLegacyTransfer>& trs);
  void initTransactionPool();
  void resetCachedBalance();
  void updateTransactionsBaseStatus();
  void updateTransfersSign();

  ITransfersObserver& m_transfersObserver;
  Crypto::PublicKey& m_viewPublicKey;
  Crypto::SecretKey& m_viewSecretKey;
  uint64_t& m_actualBalance;
  uint64_t& m_pendingBalance;
  WalletsContainer& m_walletsContainer;
  TransfersSyncronizer& m_synchronizer;
  UnlockTransactionJobs& m_unlockTransactions;
  WalletTransactions& m_transactions;
  WalletTransfers& m_transfers;
  uint32_t m_transactionSoftLockTime;
  UncommitedTransactions& uncommitedTransactions;
};

} //namespace CryptoNote
