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


#include "NodeFactory.h"

#include "NodeRpcProxy/NodeRpcProxy.h"
#include <memory>
#include <future>

namespace PaymentService {

class NodeRpcStub: public CryptoNote::INode {
public:
  virtual ~NodeRpcStub() {}
  virtual bool addObserver(CryptoNote::INodeObserver* observer) override { return true; }
  virtual bool removeObserver(CryptoNote::INodeObserver* observer) override { return true; }

  virtual void init(const Callback& callback) override { }
  virtual bool shutdown() override { return true; }

  virtual size_t getPeerCount() const override { return 0; }
  virtual uint32_t getLastLocalBlockHeight() const override { return 0; }
  virtual uint32_t getLastKnownBlockHeight() const override { return 0; }
  virtual uint32_t getLocalBlockCount() const override { return 0; }
  virtual uint32_t getKnownBlockCount() const override { return 0; }
  virtual uint64_t getLastLocalBlockTimestamp() const override { return 0; }
  virtual uint32_t getNodeHeight() const override { return 0; }
  virtual uint64_t getMinimalFee() const override{ return 0; }

  virtual void getFeeAddress() override { }

  virtual CryptoNote::BlockHeaderInfo getLastLocalBlockHeaderInfo() const override { return CryptoNote::BlockHeaderInfo(); }

  virtual void relayTransaction(const CryptoNote::Transaction& transaction, const Callback& callback) override { callback(std::error_code()); }
  virtual void getRandomOutsByAmounts(std::vector<uint64_t>&& amounts, uint64_t outsCount,
    std::vector<CryptoNote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& result, const Callback& callback) override {
  }
  virtual void getNewBlocks(std::vector<Crypto::Hash>&& knownBlockIds, std::vector<CryptoNote::block_complete_entry>& newBlocks, uint32_t& startHeight, const Callback& callback) override {
    startHeight = 0;
    callback(std::error_code());
  }
  virtual void getTransactionOutsGlobalIndices(const Crypto::Hash& transactionHash, std::vector<uint32_t>& outsGlobalIndices, const Callback& callback) override { }

  virtual void queryBlocks(std::vector<Crypto::Hash>&& knownBlockIds, uint64_t timestamp, std::vector<CryptoNote::BlockShortEntry>& newBlocks,
    uint32_t& startHeight, const Callback& callback) override {
    startHeight = 0;
    callback(std::error_code());
  };

  virtual void getPoolSymmetricDifference(std::vector<Crypto::Hash>&& knownPoolTxIds, Crypto::Hash knownBlockId, bool& isBcActual,
          std::vector<std::unique_ptr<CryptoNote::ITransactionReader>>& newTxs, std::vector<Crypto::Hash>& deletedTxIds, const Callback& callback) override {
    isBcActual = true;
    callback(std::error_code());
  }

  virtual void getBlocks(const std::vector<uint32_t>& blockHeights, std::vector<std::vector<CryptoNote::BlockDetails>>& blocks,
    const Callback& callback) override { }

  virtual void getBlocks(const std::vector<Crypto::Hash>& blockHashes, std::vector<CryptoNote::BlockDetails>& blocks,
    const Callback& callback) override { }

  virtual void getBlocks(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t blocksNumberLimit, std::vector<CryptoNote::BlockDetails>& blocks, uint32_t& blocksNumberWithinTimestamps,
    const Callback& callback) override { }

  virtual void getBlock(const uint32_t blockHeight, CryptoNote::BlockDetails &block,
    const Callback& callback) override { }

  virtual void getTransactions(const std::vector<Crypto::Hash>& transactionHashes, std::vector<CryptoNote::TransactionDetails>& transactions,
    const Callback& callback) override { }

  virtual void getPoolTransactions(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t transactionsNumberLimit, std::vector<CryptoNote::TransactionDetails>& transactions, uint64_t& transactionsNumberWithinTimestamps,
    const Callback& callback) override { }

  virtual void getTransactionsByPaymentId(const Crypto::Hash& paymentId, std::vector<CryptoNote::TransactionDetails>& transactions, 
    const Callback& callback) override { }

  virtual void getMultisignatureOutputByGlobalIndex(uint64_t amount, uint32_t gindex, CryptoNote::MultisignatureOutput& out,
    const Callback& callback) override { }

  virtual void isSynchronized(bool& syncStatus, const Callback& callback) override { }

  virtual std::string feeAddress() const override { return std::string(); }

};


class NodeInitObserver {
public:
  NodeInitObserver() {
    initFuture = initPromise.get_future();
  }

  void initCompleted(std::error_code result) {
    initPromise.set_value(result);
  }

  void waitForInitEnd() {
    std::error_code ec = initFuture.get();
    if (ec) {
      throw std::system_error(ec);
    }
    return;
  }

private:
  std::promise<std::error_code> initPromise;
  std::future<std::error_code> initFuture;
};

NodeFactory::NodeFactory() {
}

NodeFactory::~NodeFactory() {
}

CryptoNote::INode* NodeFactory::createNode(const std::string& daemonAddress, uint16_t daemonPort) {
  std::unique_ptr<CryptoNote::INode> node(new CryptoNote::NodeRpcProxy(daemonAddress, daemonPort));

  NodeInitObserver initObserver;
  node->init(std::bind(&NodeInitObserver::initCompleted, &initObserver, std::placeholders::_1));
  initObserver.waitForInitEnd();

  return node.release();
}

CryptoNote::INode* NodeFactory::createNodeStub() {
  return new NodeRpcStub();
}

}
