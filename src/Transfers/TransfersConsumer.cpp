// Copyright (c) 2021-2023, Dynex Developers
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
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#include "TransfersConsumer.h"

#include <numeric>
#include <future>
#include <algorithm>
#include <chrono>

#include "CommonTypes.h"
#include "Common/StringTools.h"
#include "Common/BlockingQueue.h"
#include "DynexCNCore/DynexCNFormatUtils.h"
#include "DynexCNCore/TransactionApi.h"

#include "IWallet.h"
#include "INode.h"

using namespace Crypto;
using namespace Logging;
using namespace Common;

std::unordered_set<Crypto::Hash> transactions_hash_seen;
std::unordered_set<Crypto::PublicKey> public_keys_seen;
std::mutex seen_mutex;
std::atomic<size_t> preprocessingWorkerLimit(8);

namespace {

using namespace DynexCN;

void checkOutputKey(
  const KeyDerivation& derivation,
  const PublicKey& key,
  size_t keyIndex,
  size_t outputIndex,
  const std::unordered_set<PublicKey>& spendKeys,
  std::unordered_map<PublicKey, std::vector<uint32_t>>& outputs) {

  PublicKey spendKey;
  underive_public_key(derivation, keyIndex, key, spendKey);

  if (spendKeys.find(spendKey) != spendKeys.end()) {
    outputs[spendKey].push_back(static_cast<uint32_t>(outputIndex));
  }

}

void findMyOutputs(
  const ITransactionReader& tx,
  const SecretKey& viewSecretKey,
  const std::unordered_set<PublicKey>& spendKeys,
  std::unordered_map<PublicKey, std::vector<uint32_t>>& outputs,
  uint64_t& derivationTimeNs,
  uint64_t& outputScanTimeNs) {

  const auto derivationStarted = std::chrono::steady_clock::now();
  auto txPublicKey = tx.getTransactionPublicKey();
  KeyDerivation derivation;

  if (!generate_key_derivation( txPublicKey, viewSecretKey, derivation)) {
    derivationTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - derivationStarted).count();
    return;
  }
  derivationTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - derivationStarted).count();

  const auto outputScanStarted = std::chrono::steady_clock::now();
  size_t keyIndex = 0;
  const auto& transactionOutputs = tx.getTransactionPrefix().outputs;

  for (size_t idx = 0; idx < transactionOutputs.size(); ++idx) {
    const auto& target = transactionOutputs[idx].target;
    if (const auto* out = boost::get<KeyOutput>(&target)) {
      checkOutputKey(derivation, out->key, keyIndex, idx, spendKeys, outputs);
      ++keyIndex;
    } else if (const auto* out = boost::get<MultisignatureOutput>(&target)) {
      for (const auto& key : out->keys) {
        checkOutputKey(derivation, key, idx, idx, spendKeys, outputs);
        ++keyIndex;
      }
    }
  }
  outputScanTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - outputScanStarted).count();
}

std::vector<Crypto::Hash> getBlockHashes(const DynexCN::CompleteBlock* blocks, size_t count) {
  std::vector<Crypto::Hash> result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i) {
    result.push_back(blocks[i].blockHash);
  }

  return result;
}

}

namespace DynexCN {

TransfersConsumer::TransfersConsumer(const DynexCN::Currency& currency, INode& node, Logging::ILogger& logger, const SecretKey& viewSecret) :
  m_viewSecret(viewSecret), m_inputOwnershipInitialized(false), m_inputOwnershipKeyCount(0), m_node(node), m_currency(currency), m_logger(logger, "TransfersConsumer") {
  updateSyncStart();
}

ITransfersSubscription& TransfersConsumer::addSubscription(const AccountSubscription& subscription) {
  if (subscription.keys.viewSecretKey != m_viewSecret) {
    throw std::runtime_error("TransfersConsumer: view secret key mismatch");
  }

  auto& res = m_subscriptions[subscription.keys.address.spendPublicKey];

  if (res.get() == nullptr) {
    res.reset(new TransfersSubscription(m_currency, m_logger.getLogger(), subscription));
    m_spendKeys.insert(subscription.keys.address.spendPublicKey);
    m_inputOwnershipInitialized = false;
    m_inputOwnershipKeyCount.store(0, std::memory_order_relaxed);
    if (m_subscriptions.size() == 1) {
      m_syncStart = res->getSyncStart();
    } else {
      auto subStart = res->getSyncStart();
      m_syncStart.height = std::min(m_syncStart.height, subStart.height);
      m_syncStart.timestamp = std::min(m_syncStart.timestamp, subStart.timestamp);
    }
  }

  return *res;
}

bool TransfersConsumer::removeSubscription(const AccountPublicAddress& address) {
  m_subscriptions.erase(address.spendPublicKey);
  m_spendKeys.erase(address.spendPublicKey);
  m_inputOwnershipInitialized = false;
  m_inputOwnershipKeyCount.store(0, std::memory_order_relaxed);
  updateSyncStart();
  return m_subscriptions.empty();
}

ITransfersSubscription* TransfersConsumer::getSubscription(const AccountPublicAddress& acc) {
  auto it = m_subscriptions.find(acc.spendPublicKey);
  return it == m_subscriptions.end() ? nullptr : it->second.get();
}

void TransfersConsumer::getSubscriptions(std::vector<AccountPublicAddress>& subscriptions) {
  for (const auto& kv : m_subscriptions) {
    subscriptions.push_back(kv.second->getAddress());
  }
}

size_t TransfersConsumer::getInputOwnershipKeyCount() const {
  return m_inputOwnershipKeyCount.load(std::memory_order_relaxed);
}

size_t TransfersConsumer::getPreprocessingWorkerCount() {
  const size_t hardwareWorkers = std::thread::hardware_concurrency();
  const size_t availableWorkers = hardwareWorkers == 0 ? preprocessingWorkerLimit.load(std::memory_order_relaxed) : hardwareWorkers;
  return std::max<size_t>(1, std::min<size_t>(availableWorkers, preprocessingWorkerLimit.load(std::memory_order_relaxed)));
}

void TransfersConsumer::setPreprocessingWorkerCount(size_t workers) {
  preprocessingWorkerLimit.store(std::max<size_t>(1, workers), std::memory_order_relaxed);
}

void TransfersConsumer::initTransactionPool(const std::unordered_set<Crypto::Hash>& uncommitedTransactions) {
  for (auto itSubscriptions = m_subscriptions.begin(); itSubscriptions != m_subscriptions.end(); ++itSubscriptions) {
    std::vector<Crypto::Hash> unconfirmedTransactions;
    itSubscriptions->second->getContainer().getUnconfirmedTransactions(unconfirmedTransactions);

    for (auto itTransactions = unconfirmedTransactions.begin(); itTransactions != unconfirmedTransactions.end(); ++itTransactions) {
      if (uncommitedTransactions.count(*itTransactions) == 0) {
        m_poolTxs.emplace(*itTransactions);
      }
    }
  }
}

void TransfersConsumer::updateSyncStart() {
  SynchronizationStart start;

  start.height =   std::numeric_limits<uint64_t>::max();
  start.timestamp = std::numeric_limits<uint64_t>::max();

  for (const auto& kv : m_subscriptions) {
    auto subStart = kv.second->getSyncStart();
    start.height = std::min(start.height, subStart.height);
    start.timestamp = std::min(start.timestamp, subStart.timestamp);
  }

  m_syncStart = start;
}

void TransfersConsumer::initializeInputOwnership() {
  if (m_inputOwnershipInitialized) {
    return;
  }

  m_keyImageOwners.clear();
  m_multisignatureOwners.clear();
  size_t indexedWallets = 0;
  for (const auto& subscription : m_subscriptions) {
    std::vector<TransferOwnershipInformation> outputs;
    subscription.second->getContainer().getOwnershipInformation(outputs);
    for (const auto& output : outputs) {
      if (output.type == TransactionTypes::OutputType::Key) {
        m_keyImageOwners.emplace(output.keyImage, subscription.first);
      } else if (output.type == TransactionTypes::OutputType::Multisignature) {
        m_multisignatureOwners.emplace(MultisignatureOutputId{output.amount, output.globalOutputIndex}, subscription.first);
      }
    }

    ++indexedWallets;
    if (indexedWallets % 1000 == 0 || indexedWallets == m_subscriptions.size()) {
      m_logger(INFO, BRIGHT_WHITE) << "Building wallet input ownership index: " << indexedWallets << '/' << m_subscriptions.size()
        << " wallets, " << m_keyImageOwners.size() << " key images";
    }
  }

  m_inputOwnershipInitialized = true;
  m_inputOwnershipKeyCount.store(m_keyImageOwners.size(), std::memory_order_relaxed);
  m_logger(INFO, BRIGHT_WHITE) << "Wallet input ownership index: " << m_keyImageOwners.size() << " key images, "
    << m_multisignatureOwners.size() << " multisignature outputs across " << m_subscriptions.size() << " wallets";
}

void TransfersConsumer::indexOwnedOutputs(const PublicKey& spendKey, const std::vector<TransactionOutputInformationIn>& outputs) {
  for (const auto& output : outputs) {
    if (output.type == TransactionTypes::OutputType::Key) {
      m_keyImageOwners.emplace(output.keyImage, spendKey);
    } else if (output.type == TransactionTypes::OutputType::Multisignature &&
               output.globalOutputIndex != UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX) {
      m_multisignatureOwners.emplace(MultisignatureOutputId{output.amount, output.globalOutputIndex}, spendKey);
    }
  }
  m_inputOwnershipKeyCount.store(m_keyImageOwners.size(), std::memory_order_relaxed);
}

SynchronizationStart TransfersConsumer::getSyncStart() {
  return m_syncStart;
}

void TransfersConsumer::onBlockchainDetach(uint32_t height) {
  m_observerManager.notify(&IBlockchainConsumerObserver::onBlockchainDetach, this, height);

  for (const auto& kv : m_subscriptions) {
    kv.second->onBlockchainDetach(height);
  }
  m_inputOwnershipInitialized = false;
  m_inputOwnershipKeyCount.store(0, std::memory_order_relaxed);
}

bool TransfersConsumer::onNewBlocks(const CompleteBlock* blocks, uint32_t startHeight, uint32_t count) {
  assert(blocks);
  assert(count > 0);

  struct Tx {
    TransactionBlockInfo blockInfo;
    const ITransactionReader* tx;
  };

  struct PreprocessedTx : Tx, PreprocessInfo {};

  std::vector<PreprocessedTx> preprocessedTransactions;
  std::mutex preprocessedTransactionsMutex;
  const auto preprocessingStarted = std::chrono::steady_clock::now();

  // A new worker group is created for every synchronization batch.  Using all
  // reported CPUs here causes large transient memory spikes (and scheduler
  // contention) on walletd hosts with many cores. Four workers keep the batch
  // parallel while bounding its memory/thread footprint.
  const size_t workers = getPreprocessingWorkerCount();

  BlockingQueue<Tx> inputQueue(workers * 2);

  std::atomic<bool> stopProcessing(false);

  auto pushingThread = std::async(std::launch::async, [&] {
    for( uint32_t i = 0; i < count && !stopProcessing; ++i) {
      const auto& block = blocks[i].block;

      if (!block.is_initialized()) {
        continue;
      }

      // filter by syncStartTimestamp
      if (m_syncStart.timestamp && block->timestamp < m_syncStart.timestamp) {
        continue;
      }

      TransactionBlockInfo blockInfo;
      blockInfo.height = startHeight + i;
      blockInfo.timestamp = block->timestamp;
      blockInfo.transactionIndex = 0; // position in block

      for (const auto& tx : blocks[i].transactions) {
        auto pubKey = tx->getTransactionPublicKey();
        if (pubKey == NULL_PUBLIC_KEY) {
          ++blockInfo.transactionIndex;
          continue;
        }

        Tx item = { blockInfo, tx.get() };
        inputQueue.push(item);
        ++blockInfo.transactionIndex;
      }
    }

    inputQueue.close();
  });

  auto processingFunction = [&] {
    Tx item;
    std::error_code ec;
    while (!stopProcessing && inputQueue.pop(item)) {
      PreprocessedTx output;
      static_cast<Tx&>(output) = item;

      ec = preprocessOutputs(item.blockInfo, *item.tx, output);
      if (ec) {
        stopProcessing = true;
        break;
      }

      std::lock_guard<std::mutex> lk(preprocessedTransactionsMutex);
      preprocessedTransactions.push_back(std::move(output));
    }
    return ec;
  };

  std::vector<std::future<std::error_code>> processingThreads;
  for (size_t i = 0; i < workers; ++i) {
    processingThreads.push_back(std::async(std::launch::async, processingFunction));
  }

  std::error_code processingError;
  for (auto& f : processingThreads) {
    try {
      std::error_code ec = f.get();
      if (!processingError && ec) {
        processingError = ec;
      }
    } catch (const std::system_error& e) {
      processingError = e.code();
    } catch (const std::exception&) {
      processingError = std::make_error_code(std::errc::operation_canceled);
    }
  }
  const auto preprocessingFinished = std::chrono::steady_clock::now();

  std::vector<Crypto::Hash> blockHashes = getBlockHashes(blocks, count);
  const auto applyStarted = std::chrono::steady_clock::now();
  if (!processingError) {
    m_observerManager.notify(&IBlockchainConsumerObserver::onBlocksAdded, this, blockHashes);

    // sort by block height and transaction index in block
    std::sort(preprocessedTransactions.begin(), preprocessedTransactions.end(), [](const PreprocessedTx& a, const PreprocessedTx& b) {
      return std::tie(a.blockInfo.height, a.blockInfo.transactionIndex) < std::tie(b.blockInfo.height, b.blockInfo.transactionIndex);
    });

    for (const auto& tx : preprocessedTransactions) {
      processTransaction(tx.blockInfo, *tx.tx, tx);
    }
  } else {
    forEachSubscription([&](TransfersSubscription& sub) {
      sub.onError(processingError, startHeight);
    });

    return false;
  }

  auto newHeight = startHeight + count - 1;
  forEachSubscription([newHeight](TransfersSubscription& sub) {
    sub.advanceHeight(newHeight);
  });
  const auto applyFinished = std::chrono::steady_clock::now();
  uint64_t derivationTimeNs = 0;
  uint64_t outputScanTimeNs = 0;
  uint64_t matchedOutputTimeNs = 0;
  uint64_t globalIndicesTimeNs = 0;
  uint64_t createTransfersTimeNs = 0;
  for (const auto& tx : preprocessedTransactions) {
    derivationTimeNs += tx.derivationTimeNs;
    outputScanTimeNs += tx.outputScanTimeNs;
    matchedOutputTimeNs += tx.matchedOutputTimeNs;
    globalIndicesTimeNs += tx.globalIndicesTimeNs;
    createTransfersTimeNs += tx.createTransfersTimeNs;
  }
  m_logger(DEBUGGING) << "Wallet consumer timing: blocks " << count << ", transactions " << preprocessedTransactions.size()
    << ", preprocess " << std::chrono::duration_cast<std::chrono::milliseconds>(preprocessingFinished - preprocessingStarted).count() << " ms"
    << ", apply-and-advance " << std::chrono::duration_cast<std::chrono::milliseconds>(applyFinished - applyStarted).count() << " ms"
    << ", worker-cpu derivation " << derivationTimeNs / 1000000 << " ms"
    << ", output-scan " << outputScanTimeNs / 1000000 << " ms"
    << ", matched-output " << matchedOutputTimeNs / 1000000 << " ms"
    << ", global-indices " << globalIndicesTimeNs / 1000000 << " ms"
    << ", create-transfers " << createTransfersTimeNs / 1000000 << " ms";

  return true;
}

std::error_code TransfersConsumer::onPoolUpdated(const std::vector<std::unique_ptr<ITransactionReader>>& addedTransactions, const std::vector<Hash>& deletedTransactions) {
  TransactionBlockInfo unconfirmedBlockInfo;
  unconfirmedBlockInfo.timestamp = 0; 
  unconfirmedBlockInfo.height = WALLET_UNCONFIRMED_TRANSACTION_HEIGHT;

  std::error_code processingError;
  for (auto& cryptonoteTransaction : addedTransactions) {
    m_poolTxs.emplace(cryptonoteTransaction->getTransactionHash());
    processingError = processTransaction(unconfirmedBlockInfo, *cryptonoteTransaction.get());
    if (processingError) {
      for (auto& sub : m_subscriptions) {
        sub.second->onError(processingError, WALLET_UNCONFIRMED_TRANSACTION_HEIGHT);
      }

      return processingError;
    }
  }
  
  for (auto& deletedTxHash : deletedTransactions) {
    m_poolTxs.erase(deletedTxHash);

    m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionDeleteBegin, this, deletedTxHash);
    for (auto& sub : m_subscriptions) {
      sub.second->deleteUnconfirmedTransaction(*reinterpret_cast<const Hash*>(&deletedTxHash));
    }

    m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionDeleteEnd, this, deletedTxHash);
  }

  return std::error_code();
}

const std::unordered_set<Crypto::Hash>& TransfersConsumer::getKnownPoolTxIds() const {
  return m_poolTxs;
}

std::error_code TransfersConsumer::addUnconfirmedTransaction(const ITransactionReader& transaction) {
  TransactionBlockInfo unconfirmedBlockInfo;
  unconfirmedBlockInfo.height = WALLET_UNCONFIRMED_TRANSACTION_HEIGHT;
  unconfirmedBlockInfo.timestamp = 0;
  unconfirmedBlockInfo.transactionIndex = 0;

  return processTransaction(unconfirmedBlockInfo, transaction);
}

void TransfersConsumer::removeUnconfirmedTransaction(const Crypto::Hash& transactionHash) {
  m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionDeleteBegin, this, transactionHash);
  for (auto& subscription : m_subscriptions) {
    subscription.second->deleteUnconfirmedTransaction(transactionHash);
  }
  m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionDeleteEnd, this, transactionHash);
}

void TransfersConsumer::addPublicKeysSeen(const Crypto::Hash& transactionHash, const Crypto::PublicKey& outputKey) {
	std::lock_guard<std::mutex> lk(seen_mutex);
    transactions_hash_seen.insert(transactionHash);
    public_keys_seen.insert(outputKey);
}

std::error_code TransfersConsumer::createTransfers(
  const AccountKeys& account,
  const TransactionBlockInfo& blockInfo,
  const ITransactionReader& tx,
  const std::vector<uint32_t>& outputs,
  const std::vector<uint32_t>& globalIdxs,
  std::vector<TransactionOutputInformationIn>& transfers) {

  auto txPubKey = tx.getTransactionPublicKey();
  auto txHash = tx.getTransactionHash();
  std::vector<PublicKey> temp_keys;

  for (auto idx : outputs) {

    if (idx >= tx.getOutputCount()) {
      return std::make_error_code(std::errc::argument_out_of_domain);
    }

    auto outType = tx.getOutputType(size_t(idx));

    if (
      outType != TransactionTypes::OutputType::Key &&
      outType != TransactionTypes::OutputType::Multisignature) {
      continue;
    }

    TransactionOutputInformationIn info;

    info.type = outType;
    info.transactionPublicKey = txPubKey;
    info.outputInTransaction = idx;
    info.globalOutputIndex = (blockInfo.height == WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) ?
      UNCONFIRMED_TRANSACTION_GLOBAL_OUTPUT_INDEX : globalIdxs[idx];

    if (outType == TransactionTypes::OutputType::Key) {
      uint64_t amount;
      KeyOutput out;
      tx.getOutput(idx, out, amount);

      DynexCN::KeyPair in_ephemeral;
      DynexCN::generate_key_image_helper(
        account,
        txPubKey,
        idx,
        in_ephemeral,
        info.keyImage);

      assert(out.key == reinterpret_cast<const PublicKey&>(in_ephemeral.publicKey));

      temp_keys.push_back(out.key);
      info.amount = amount;
      info.outputKey = out.key;

    } else if (outType == TransactionTypes::OutputType::Multisignature) {
      uint64_t amount;
      MultisignatureOutput out;
      tx.getOutput(idx, out, amount);

      for (const auto& key : out.keys) {
        temp_keys.push_back(key);
      }
      info.amount = amount;
      info.requiredSignatures = out.requiredSignatureCount;
    }

    transfers.push_back(info);
  }

  // Key-image generation above is expensive and independent per transaction.
  // Serialize only the cross-transaction duplicate check and insertion, not
  // the cryptographic transfer construction performed by all four workers.
  {
    std::lock_guard<std::mutex> lk(seen_mutex);
    if (transactions_hash_seen.find(txHash) == transactions_hash_seen.end()) {
      std::unordered_set<PublicKey> transactionKeys;
      transactionKeys.reserve(temp_keys.size());
      for (const auto& key : temp_keys) {
        if (public_keys_seen.find(key) != public_keys_seen.end()) {
          m_logger(ERROR, BRIGHT_RED) << "Failed to process transaction " << Common::podToHex(txHash) << ": duplicate output key is found";
          return std::error_code();
        }
        if (!transactionKeys.emplace(key).second) {
          m_logger(ERROR, BRIGHT_RED) << "Failed to process transaction " << Common::podToHex(txHash) << ": the same output key is present more than once";
          return std::error_code();
        }
      }

      public_keys_seen.insert(transactionKeys.begin(), transactionKeys.end());
    }
    transactions_hash_seen.emplace(txHash);
  }

  return std::error_code();
}

std::error_code TransfersConsumer::preprocessOutputs(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx, PreprocessInfo& info) {
  std::unordered_map<PublicKey, std::vector<uint32_t>> outputs;
  findMyOutputs(tx, m_viewSecret, m_spendKeys, outputs, info.derivationTimeNs, info.outputScanTimeNs);
  if (outputs.empty()) {
    return std::error_code();
  }

  const auto matchedOutputStarted = std::chrono::steady_clock::now();
  std::error_code errorCode;
  auto txHash = tx.getTransactionHash();
  if (blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
    const auto globalIndicesStarted = std::chrono::steady_clock::now();
    errorCode = getGlobalIndices(reinterpret_cast<const Hash&>(txHash), info.globalIdxs);
    info.globalIndicesTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - globalIndicesStarted).count();
    if (errorCode) {
      return errorCode;
    }
  }

  const auto createTransfersStarted = std::chrono::steady_clock::now();
  for (const auto& kv : outputs) {
    auto it = m_subscriptions.find(kv.first);
    if (it != m_subscriptions.end()) {
      auto& transfers = info.outputs[kv.first];
      errorCode = createTransfers(it->second->getKeys(), blockInfo, tx, kv.second, info.globalIdxs, transfers);
      if (errorCode) {
        return errorCode;
      }
    }
  }
  info.createTransfersTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - createTransfersStarted).count();

  info.matchedOutputTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - matchedOutputStarted).count();

  return std::error_code();
}

std::error_code TransfersConsumer::processTransaction(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx) {
  PreprocessInfo info;
  auto ec = preprocessOutputs(blockInfo, tx, info);
  if (ec) {
    return ec;
  }

  processTransaction(blockInfo, tx, info);
  return std::error_code();
}

void TransfersConsumer::processTransaction(const TransactionBlockInfo& blockInfo, const ITransactionReader& tx, const PreprocessInfo& info) {
  std::vector<TransactionOutputInformationIn> emptyOutputs;
  std::vector<ITransfersContainer*> transactionContainers;
  bool someContainerUpdated = false;

  // Generating transactions have no spendable inputs, so only subscriptions
  // with a matching output can possibly change. Avoid probing every sub-wallet
  // for every block; this is especially important for large walletd containers.
  if (tx.getInputCount() == 1 && tx.getInputType(0) == TransactionTypes::InputType::Generating) {
    for (const auto& output : info.outputs) {
      auto subscription = m_subscriptions.find(output.first);
      if (subscription == m_subscriptions.end()) {
        continue;
      }

      bool containerContainsTx;
      bool containerUpdated;
      processOutputs(blockInfo, *subscription->second, tx, output.second, info.globalIdxs, containerContainsTx, containerUpdated);
      someContainerUpdated = someContainerUpdated || containerUpdated;
      if (containerUpdated && m_inputOwnershipInitialized) {
        indexOwnedOutputs(output.first, output.second);
      }
      if (containerContainsTx) {
        transactionContainers.emplace_back(&subscription->second->getContainer());
      }
    }

    if (someContainerUpdated) {
      m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionUpdated, this, tx.getTransactionHash(), transactionContainers);
    }
    return;
  }

  initializeInputOwnership();

  // Route inputs directly to the subscription that owns the referenced output
  // instead of probing every wallet with transaction history.
  std::unordered_set<PublicKey> affectedSpendKeys;
  std::vector<KeyImage> confirmedSpentKeyImages;
  std::vector<MultisignatureOutputId> confirmedSpentMultisignatureOutputs;
  for (size_t inputIndex = 0; inputIndex < tx.getInputCount(); ++inputIndex) {
    const auto inputType = tx.getInputType(inputIndex);
    if (inputType == TransactionTypes::InputType::Key) {
      KeyInput input;
      tx.getInput(inputIndex, input);
      auto owner = m_keyImageOwners.find(input.keyImage);
      if (owner != m_keyImageOwners.end()) {
        affectedSpendKeys.insert(owner->second);
        if (blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
          confirmedSpentKeyImages.push_back(input.keyImage);
        }
      }
    } else if (inputType == TransactionTypes::InputType::Multisignature) {
      MultisignatureInput input;
      tx.getInput(inputIndex, input);
      const MultisignatureOutputId outputId{input.amount, input.outputIndex};
      auto owner = m_multisignatureOwners.find(outputId);
      if (owner != m_multisignatureOwners.end()) {
        affectedSpendKeys.insert(owner->second);
        if (blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
          confirmedSpentMultisignatureOutputs.push_back(outputId);
        }
      }
    }
  }

  // Incoming-output detection is still global in preprocessOutputs. Add every
  // matched recipient so transactions that only receive funds are processed.
  for (const auto& output : info.outputs) {
    affectedSpendKeys.insert(output.first);
  }

  for (const auto& spendKey : affectedSpendKeys) {
    auto subscription = m_subscriptions.find(spendKey);
    if (subscription == m_subscriptions.end()) {
      continue;
    }

    auto it = info.outputs.find(spendKey);
    auto& subscriptionOutputs = (it == info.outputs.end()) ? emptyOutputs : it->second;

    bool containerContainsTx;
    bool containerUpdated;
    processOutputs(blockInfo, *subscription->second, tx, subscriptionOutputs, info.globalIdxs, containerContainsTx, containerUpdated);
    someContainerUpdated = someContainerUpdated || containerUpdated;
    if (containerUpdated && it != info.outputs.end()) {
      indexOwnedOutputs(spendKey, it->second);
    }
    if (containerContainsTx) {
      transactionContainers.emplace_back(&subscription->second->getContainer());
    }
  }

  // Keep the routing index proportional to currently relevant outputs. The
  // containers retain complete transaction history; only confirmed-spent
  // lookup entries are removed here. A detach invalidates and rebuilds this
  // index from container state.
  for (const auto& keyImage : confirmedSpentKeyImages) {
    m_keyImageOwners.erase(keyImage);
  }
  for (const auto& outputId : confirmedSpentMultisignatureOutputs) {
    m_multisignatureOwners.erase(outputId);
  }
  m_inputOwnershipKeyCount.store(m_keyImageOwners.size(), std::memory_order_relaxed);

  if (someContainerUpdated) {
    m_observerManager.notify(&IBlockchainConsumerObserver::onTransactionUpdated, this, tx.getTransactionHash(), transactionContainers);
  }
}

void TransfersConsumer::processOutputs(const TransactionBlockInfo& blockInfo, TransfersSubscription& sub, const ITransactionReader& tx,
  const std::vector<TransactionOutputInformationIn>& transfers, const std::vector<uint32_t>& globalIdxs, bool& contains, bool& updated) {

  TransactionInformation subscribtionTxInfo;
  contains = sub.getContainer().getTransactionInformation(tx.getTransactionHash(), subscribtionTxInfo);
  updated = false;

  if (contains) {
    if (subscribtionTxInfo.blockHeight == WALLET_UNCONFIRMED_TRANSACTION_HEIGHT && blockInfo.height != WALLET_UNCONFIRMED_TRANSACTION_HEIGHT) {
      // pool->blockchain
      sub.markTransactionConfirmed(blockInfo, tx.getTransactionHash(), globalIdxs);
      updated = true;
    } else {
      assert(subscribtionTxInfo.blockHeight == blockInfo.height);
    }
  } else {
    updated = sub.addTransaction(blockInfo, tx, transfers);
    contains = updated;
  }
}

std::error_code TransfersConsumer::getGlobalIndices(const Hash& transactionHash, std::vector<uint32_t>& outsGlobalIndices) {  
  std::promise<std::error_code> prom;
  std::future<std::error_code> f = prom.get_future();

  INode::Callback cb = [&prom](std::error_code ec) { 
    std::promise<std::error_code> p(std::move(prom));
    p.set_value(ec);
  };

  outsGlobalIndices.clear();
  m_node.getTransactionOutsGlobalIndices(transactionHash, outsGlobalIndices, cb);

  return f.get();
}

}
