// Copyright (c) 2022-2023, Dynex Developers
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
// Copyright (c) 2012-2017 The CN developers
// Copyright (c) 2012-2017 The Bytecoin developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2014-2018 The Monero project
// Copyright (c) 2014-2018 The Forknote developers
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2022 The Karbo developers

#include "DynexCNWrapper.h"
#include "DynexCNCore/DynexCNBasicImpl.h"
#include "DynexCNCore/DynexCNFormatUtils.h"
#include "DynexCNCore/Currency.h"
#include "NodeRpcProxy/NodeRpcProxy.h"
#include "DynexCNCore/CoreConfig.h"
#include "P2p/NetNodeConfig.h"
#include "DynexCNCore/Core.h"
#include "DynexCNProtocol/DynexCNProtocolHandler.h"
#include "InProcessNode/InProcessNode.h"
#include "P2p/NetNode.h"
#include "WalletLegacy/WalletLegacy.h"
#include "LoggerAdapter.h"
#include "System/Dispatcher.h"
#include "CheckpointsData.h"

namespace WalletGui {

namespace {

bool parsePaymentId(const std::string& payment_id_str, Crypto::Hash& payment_id) {
  return DynexCN::parsePaymentId(payment_id_str, payment_id);
}

std::string convertPaymentId(const std::string& paymentIdString) {
  if (paymentIdString.empty()) {
    return "";
  }

  Crypto::Hash paymentId;
  if (!parsePaymentId(paymentIdString, paymentId)) {
    std::stringstream errorStr;
    errorStr << "Payment id has invalid format: \"" + paymentIdString + "\", expected 64-character string";
    throw std::runtime_error(errorStr.str());
  }

  std::vector<uint8_t> extra;
  DynexCN::BinaryArray extraNonce;
  DynexCN::setPaymentIdToTransactionExtraNonce(extraNonce, paymentId);
  if (!DynexCN::addExtraNonceToTransactionExtra(extra, extraNonce)) {
    std::stringstream errorStr;
    errorStr << "Something went wrong with payment_id. Please check its format: \"" + paymentIdString + "\", expected 64-character string";
    throw std::runtime_error(errorStr.str());
  }

  return std::string(extra.begin(), extra.end());
}

bool extractExtra(const std::string& extra, std::vector<DynexCN::TransactionExtraField>& extraFields) {
  return DynexCN::parseTransactionExtra(extra, extraFields);
}

std::string extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& extraFields) {
  std::string result;
  DynexCN::TransactionExtraNonce extraNonce;
  if (DynexCN::findTransactionExtraFieldByType(extraFields, extraNonce)) {
    Crypto::Hash paymentIdHash;
    if (DynexCN::getPaymentIdFromTransactionExtraNonce(extraNonce.nonce, paymentIdHash)) {
      unsigned char* buff = reinterpret_cast<unsigned char *>(&paymentIdHash);
      for (size_t i = 0; i < sizeof(paymentIdHash); ++i) {
        result.push_back("0123456789ABCDEF"[buff[i] >> 4]);
        result.push_back("0123456789ABCDEF"[buff[i] & 15]);
      }
    }
  }
  return result;
}

std::string extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) {
  DynexCN::TransactionExtraFromAddress value;
  if (DynexCN::findTransactionExtraFieldByType(extraFields, value)) {
    return DynexCN::getAccountAddressAsStr(value.address);
  }
  return {};
}

std::vector<std::pair<std::string, int64_t>> extractToAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) {
  std::vector<std::pair<std::string, int64_t>> result;
  std::string to_address;

  for (const DynexCN::TransactionExtraField& field : extraFields) {
    if (typeid(DynexCN::TransactionExtraToAddress) == field.type()) {
      to_address = DynexCN::getAccountAddressAsStr(boost::get<DynexCN::TransactionExtraToAddress>(field).address);
    } else if (typeid(DynexCN::TransactionExtraAmount) == field.type()) {
      result.push_back(std::make_pair(to_address, DynexCN::getAmountInt64(boost::get<DynexCN::TransactionExtraAmount>(field).amount)));
    }
  }
  return result;
}

}

Node::~Node() {
}

class RpcNode : DynexCN::INodeObserver, public Node {
public:
  RpcNode(const DynexCN::Currency& currency, INodeCallback& callback, const std::string& nodeHost, unsigned short nodePort) :
    m_callback(callback),
    m_currency(currency),
    m_node(nodeHost, nodePort) {
    m_node.addObserver(this);
  }

  ~RpcNode() override {
  }

  void init(const std::function<void(std::error_code)>& callback) override {
    m_node.init(callback);
  }

  void deinit() override {
    m_node.removeObserver(this);
  }

  std::string convertPaymentId(const std::string& paymentIdString) const override {
    return WalletGui::convertPaymentId(paymentIdString);
  }

  bool extractExtra(const std::string& extra, std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractExtra(extra, extraFields);
  }

  std::string extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractPaymentId(extraFields);
  }

  std::string extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractFromAddress(extraFields);
  }

  std::vector<std::pair<std::string, int64_t>> extractToAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractToAddress(extraFields);
  }

  uint32_t getLastKnownBlockHeight() const override {
    return m_node.getLastKnownBlockHeight();
  }

  uint32_t getLastLocalBlockHeight() const override {
    return m_node.getLastLocalBlockHeight();
  }

  uint64_t getLastLocalBlockTimestamp() const override {
    return m_node.getLastLocalBlockTimestamp();
  }

  size_t getPeerCount() const override {
    return m_node.getPeerCount();
  }

  uint64_t getMinimalFee() const override {
    return m_node.getMinimalFee();
  }

  DynexCN::IWalletLegacy* createWallet() override {
    return new DynexCN::WalletLegacy(m_currency, m_node, LoggerAdapter::instance().getLoggerManager());
  }

private:
  INodeCallback& m_callback;
  const DynexCN::Currency& m_currency;
  DynexCN::NodeRpcProxy m_node;

  void peerCountUpdated(size_t count) override {
    m_callback.peerCountUpdated(*this, count);
  }

  void localBlockchainUpdated(uint32_t height) override {
    m_callback.localBlockchainUpdated(*this, height);
  }

  void lastKnownBlockHeightUpdated(uint32_t height) override {
    m_callback.lastKnownBlockHeightUpdated(*this, height);
  }
};

class InprocessNode : DynexCN::INodeObserver, public Node {
public:
  InprocessNode(const DynexCN::Currency& currency, Logging::LoggerManager& logManager, const DynexCN::CoreConfig& coreConfig,
    const DynexCN::NetNodeConfig& netNodeConfig, INodeCallback& callback) :
    m_currency(currency),
    m_dispatcher(),
    m_callback(callback),
    m_coreConfig(coreConfig),
    m_logManager(logManager),
    m_netNodeConfig(netNodeConfig),
    m_core(currency, NULL, logManager, false),
    m_protocolHandler(currency, m_dispatcher, m_core, nullptr, logManager),
    m_nodeServer(m_dispatcher, m_protocolHandler, logManager),
    m_node(m_core, m_protocolHandler) {
    m_core.set_cryptonote_protocol(&m_protocolHandler);
    m_protocolHandler.set_p2p_endpoint(&m_nodeServer);

    DynexCN::Checkpoints checkpoints(logManager);
    if (!netNodeConfig.getTestnet()) {
      for (const auto& cp : DynexCN::CHECKPOINTS) {
        checkpoints.add_checkpoint(cp.height, cp.blockId);
      }
    }
    checkpoints.load_checkpoints_from_remote(netNodeConfig.getTestnet());
    m_core.set_checkpoints(std::move(checkpoints));
  }

  ~InprocessNode() override {

  }

  void init(const std::function<void(std::error_code)>& callback) override {
    try {
      if (!m_core.init(m_coreConfig, true)) {
        callback(make_error_code(DynexCN::error::NOT_INITIALIZED));
        return;
      }

      if (!m_nodeServer.init(m_netNodeConfig)) {
        callback(make_error_code(DynexCN::error::NOT_INITIALIZED));
        return;
      }
    } catch (std::runtime_error&) {
      callback(make_error_code(DynexCN::error::NOT_INITIALIZED));
      return;
    }

    m_node.init([this, callback](std::error_code ec) {
      m_node.addObserver(this);
      callback(ec);
    });

    m_nodeServer.run();

    //deinitialize components
    LoggerAdapter::instance().log("Shutting down...");
    m_node.shutdown();
    m_core.deinit();
    m_nodeServer.deinit();
    m_core.set_cryptonote_protocol(NULL);
    m_protocolHandler.set_p2p_endpoint(NULL);
  }

  void deinit() override {
    m_nodeServer.sendStopSignal();
  }

  std::string convertPaymentId(const std::string& paymentIdString) const override {
    return WalletGui::convertPaymentId(paymentIdString);
  }

  bool extractExtra(const std::string& extra, std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractExtra(extra, extraFields);
  }

  std::string extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractPaymentId(extraFields);
  }

  std::string extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractFromAddress(extraFields);
  }

  std::vector<std::pair<std::string, int64_t>> extractToAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const override {
    return WalletGui::extractToAddress(extraFields);
  }

  uint32_t getLastKnownBlockHeight() const override {
    return m_node.getLastKnownBlockHeight();
  }

  uint32_t getLastLocalBlockHeight() const override {
    return m_node.getLastLocalBlockHeight();
  }

  uint64_t getLastLocalBlockTimestamp() const override {
    return m_node.getLastLocalBlockTimestamp();
  }

  size_t getPeerCount() const override {
    return m_node.getPeerCount();
  }

  uint64_t getMinimalFee() const override {
    return m_node.getMinimalFee();
  }

  DynexCN::IWalletLegacy* createWallet() override {
    return new DynexCN::WalletLegacy(m_currency, m_node, m_logManager);
  }

private:
  INodeCallback& m_callback;
  const DynexCN::Currency& m_currency;
  System::Dispatcher m_dispatcher;
  DynexCN::CoreConfig m_coreConfig;
  DynexCN::NetNodeConfig m_netNodeConfig;
  DynexCN::core m_core;
  DynexCN::DynexCNProtocolHandler m_protocolHandler;
  DynexCN::NodeServer m_nodeServer;
  DynexCN::InProcessNode m_node;
  std::future<bool> m_nodeServerFuture;
  Logging::LoggerManager& m_logManager;

  void peerCountUpdated(size_t count) override {
    m_callback.peerCountUpdated(*this, count);
  }

  void localBlockchainUpdated(uint32_t height) override {
    m_callback.localBlockchainUpdated(*this, height);
  }

  void lastKnownBlockHeightUpdated(uint32_t height) override {
    m_callback.lastKnownBlockHeightUpdated(*this, height);
  }
};

Node* createRpcNode(const DynexCN::Currency& currency, INodeCallback& callback, const std::string& nodeHost, unsigned short nodePort) {
  return new RpcNode(currency, callback, nodeHost, nodePort);
}

Node* createInprocessNode(const DynexCN::Currency& currency, Logging::LoggerManager& logManager,
  const DynexCN::CoreConfig& coreConfig, const DynexCN::NetNodeConfig& netNodeConfig, INodeCallback& callback) {
  return new InprocessNode(currency, logManager, coreConfig, netNodeConfig, callback);
}

}
