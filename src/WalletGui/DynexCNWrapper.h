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

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <system_error>

#include <DynexCNCore/TransactionExtra.h>

namespace DynexCN {

class INode;
class IWalletLegacy;
class Currency;
class CoreConfig;
class NetNodeConfig;

}

namespace Logging {
  class LoggerManager;
}

namespace WalletGui {

class Node {
public:
  virtual ~Node() = 0;
  virtual void init(const std::function<void(std::error_code)>& callback) = 0;
  virtual void deinit() = 0;
  
  virtual std::string convertPaymentId(const std::string& paymentIdString) const = 0;
  virtual bool extractExtra(const std::string& extra, std::vector<DynexCN::TransactionExtraField>& extraFields) const = 0;
  virtual std::string extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& extraFields) const = 0;
  virtual std::string extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const = 0;
  virtual std::vector<std::pair<std::string, int64_t>> extractToAddress(const std::vector<DynexCN::TransactionExtraField>& extraFields) const = 0;

  virtual uint32_t getLastKnownBlockHeight() const = 0;
  virtual uint32_t getLastLocalBlockHeight() const = 0;
  virtual uint64_t getLastLocalBlockTimestamp() const = 0;
  virtual size_t getPeerCount() const = 0;
  virtual uint64_t getMinimalFee() const = 0;

  virtual DynexCN::IWalletLegacy* createWallet() = 0;
};

class INodeCallback {
public:
  virtual void peerCountUpdated(Node& node, size_t count) = 0;
  virtual void localBlockchainUpdated(Node& node, uint32_t height) = 0;
  virtual void lastKnownBlockHeightUpdated(Node& node, uint32_t height) = 0;
};

Node* createRpcNode(const DynexCN::Currency& currency, INodeCallback& callback, const std::string& nodeHost, unsigned short nodePort);
Node* createInprocessNode(const DynexCN::Currency& currency, Logging::LoggerManager& logManager,
  const DynexCN::CoreConfig& coreConfig, const DynexCN::NetNodeConfig& netNodeConfig, INodeCallback& callback);

}
