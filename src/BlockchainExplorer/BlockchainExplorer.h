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


#pragma once

#include <mutex>
#include <atomic>
#include <unordered_set>

#include "IBlockchainExplorer.h"
#include "INode.h"

#include "Common/ObserverManager.h"
#include "BlockchainExplorerErrors.h"

#include "Wallet/WalletAsyncContextCounter.h"

#include "Logging/LoggerRef.h"

namespace CryptoNote {

class BlockchainExplorer : public IBlockchainExplorer, public INodeObserver {
public:
  BlockchainExplorer(INode& node, Logging::ILogger& logger);

  BlockchainExplorer(const BlockchainExplorer&) = delete;
  BlockchainExplorer(BlockchainExplorer&&) = delete;

  BlockchainExplorer& operator=(const BlockchainExplorer&) = delete;
  BlockchainExplorer& operator=(BlockchainExplorer&&) = delete;

  virtual ~BlockchainExplorer();
    
  virtual bool addObserver(IBlockchainObserver* observer) override;
  virtual bool removeObserver(IBlockchainObserver* observer) override;

  virtual bool getBlocks(const std::vector<uint32_t>& blockHeights, std::vector<std::vector<BlockDetails>>& blocks) override;
  virtual bool getBlocks(const std::vector<Crypto::Hash>& blockHashes, std::vector<BlockDetails>& blocks) override;
  virtual bool getBlocks(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t blocksNumberLimit, std::vector<BlockDetails>& blocks, uint32_t& blocksNumberWithinTimestamps) override;

  virtual bool getBlockchainTop(BlockDetails& topBlock) override;

  virtual bool getTransactions(const std::vector<Crypto::Hash>& transactionHashes, std::vector<TransactionDetails>& transactions) override;
  virtual bool getTransactionsByPaymentId(const Crypto::Hash& paymentId, std::vector<TransactionDetails>& transactions) override;
  virtual bool getPoolTransactions(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t transactionsNumberLimit, std::vector<TransactionDetails>& transactions, uint64_t& transactionsNumberWithinTimestamps) override;
  virtual bool getPoolState(const std::vector<Crypto::Hash>& knownPoolTransactionHashes, Crypto::Hash knownBlockchainTop, bool& isBlockchainActual, std::vector<TransactionDetails>& newTransactions, std::vector<Crypto::Hash>& removedTransactions) override;

  virtual uint64_t getRewardBlocksWindow() override;
  virtual uint64_t getFullRewardMaxBlockSize(uint8_t majorVersion) override;

  virtual bool isSynchronized() override;

  virtual void init() override;
  virtual void shutdown() override;

  virtual void poolChanged() override;
  virtual void blockchainSynchronized(uint32_t topHeight) override;
  virtual void localBlockchainUpdated(uint32_t height) override;

  typedef WalletAsyncContextCounter AsyncContextCounter;

private:
  void poolUpdateEndHandler();

  class PoolUpdateGuard {
  public:
    PoolUpdateGuard();

    bool beginUpdate();
    bool endUpdate();

  private:
    enum class State {
      NONE,
      UPDATING,
      UPDATE_REQUIRED
    };

    std::atomic<State> m_state;
  };

  enum State {
    NOT_INITIALIZED,
    INITIALIZED
  };

  BlockDetails knownBlockchainTop;
  uint32_t knownBlockchainTopHeight;
  std::unordered_set<Crypto::Hash> knownPoolState;

  std::atomic<State> state;
  std::atomic<bool> synchronized;
  std::atomic<uint32_t> observersCounter;
  Tools::ObserverManager<IBlockchainObserver> observerManager;

  std::mutex mutex;

  INode& node;
  Logging::LoggerRef logger;

  AsyncContextCounter asyncContextCounter;
  PoolUpdateGuard poolUpdateGuard;
};
}
