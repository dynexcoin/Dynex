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

#include <QObject>
#include <QThread>

#include <INode.h>
#include <IWalletLegacy.h>

#include "DynexCNWrapper.h"

namespace DynexCN {

class Currency;

}

namespace Logging {
  class LoggerManager;
}

namespace WalletGui {

class InProcessNodeInitializer;

class NodeAdapter : public QObject, public INodeCallback {
  Q_OBJECT
  Q_DISABLE_COPY(NodeAdapter)

public:
  static NodeAdapter& instance();

  quintptr getPeerCount() const;
  std::string convertPaymentId(const QString& _payment_id_string) const;

  bool extractExtra(const std::string& _extra, std::vector<DynexCN::TransactionExtraField>& _field) const;
  QString extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& _fields) const;
  QString extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& _fields) const;
  std::vector<std::pair<std::string, int64_t>> extractToAddress(const std::vector<DynexCN::TransactionExtraField>& _fields) const;

  DynexCN::IWalletLegacy* createWallet() const;

  bool init();
  void deinit();
  quint32 getLastKnownBlockHeight() const;
  quint32 getLastLocalBlockHeight() const;
  quint64 getMinimalFee() const;
  QDateTime getLastLocalBlockTimestamp() const;
  void peerCountUpdated(Node& _node, size_t _count) Q_DECL_OVERRIDE;
  void localBlockchainUpdated(Node& _node, uint32_t _height) Q_DECL_OVERRIDE;
  void lastKnownBlockHeightUpdated(Node& _node, uint32_t _height) Q_DECL_OVERRIDE;

private:
  Node* m_node;
  QThread m_nodeInitializerThread;
  InProcessNodeInitializer* m_nodeInitializer;

  NodeAdapter();
  ~NodeAdapter();

  bool initRPCNode(QUrl rpcnode);
  bool initInProcessNode();
  DynexCN::CoreConfig makeCoreConfig() const;
  DynexCN::NetNodeConfig makeNetNodeConfig() const;

Q_SIGNALS:
  void localBlockchainUpdatedSignal(quint32 _height);
  void lastKnownBlockHeightUpdatedSignal(quint32 _height);
  void nodeInitCompletedSignal();
  void peerCountUpdatedSignal(quintptr _count);
  void initNodeSignal(Node** _node, INodeCallback* _callback, const DynexCN::CoreConfig& _coreConfig, const DynexCN::NetNodeConfig& _netNodeConfig);
  void deinitNodeSignal(Node** _node);
};

}
