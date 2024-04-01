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

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <DynexCNCore/CoreConfig.h>
#include <P2p/NetNodeConfig.h>
#include <Wallet/WalletErrors.h>

#include "CurrencyAdapter.h"
#include "LoggerAdapter.h"
#include "NodeAdapter.h"
#include "Settings.h"

#include <curl/curl.h>

namespace WalletGui {

namespace {

// curl return value function
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

std::vector<std::string> convertStringListToVector(const QStringList& list) {
  std::vector<std::string> result;
  Q_FOREACH (const QString& item, list) {
    result.push_back(item.toStdString());
  }

  return result;
}

}

class InProcessNodeInitializer : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(InProcessNodeInitializer)

Q_SIGNALS:
  void nodeInitCompletedSignal();
  void nodeInitFailedSignal(int _errorCode);
  void nodeDeinitCompletedSignal();

public:
  InProcessNodeInitializer(QObject* _parent = nullptr) {
  }

  ~InProcessNodeInitializer() {
  }

  void start(Node** _node, INodeCallback* _callback, const DynexCN::CoreConfig& _coreConfig, const DynexCN::NetNodeConfig& _netNodeConfig) {
    (*_node) = createInprocessNode(CurrencyAdapter::instance().getCurrency(), LoggerAdapter::instance().getLoggerManager(),
      _coreConfig, _netNodeConfig, *_callback);
    try {
      (*_node)->init([this](std::error_code _err) {
          if (_err) {
            Q_EMIT nodeInitFailedSignal(_err.value());
            QCoreApplication::processEvents();
            return;
          }

          Q_EMIT nodeInitCompletedSignal();
          QCoreApplication::processEvents();
        });
    } catch (std::runtime_error&) {
      Q_EMIT nodeInitFailedSignal(DynexCN::error::INTERNAL_WALLET_ERROR);
      QCoreApplication::processEvents();
      return;
    }

    delete *_node;
    *_node = nullptr;
    Q_EMIT nodeDeinitCompletedSignal();
  }

  void stop(Node** _node) {
    Q_CHECK_PTR(*_node);
    (*_node)->deinit();
  }
};

NodeAdapter& NodeAdapter::instance() {
  static NodeAdapter inst;
  return inst;
}

NodeAdapter::NodeAdapter() : QObject(), m_node(nullptr), m_nodeInitializerThread(), m_nodeInitializer(new InProcessNodeInitializer) {
  m_nodeInitializer->moveToThread(&m_nodeInitializerThread);

  qRegisterMetaType<DynexCN::CoreConfig>("DynexCN::CoreConfig");
  qRegisterMetaType<DynexCN::NetNodeConfig>("DynexCN::NetNodeConfig");

  connect(m_nodeInitializer, &InProcessNodeInitializer::nodeInitCompletedSignal, this, &NodeAdapter::nodeInitCompletedSignal, Qt::QueuedConnection);
  connect(this, &NodeAdapter::initNodeSignal, m_nodeInitializer, &InProcessNodeInitializer::start, Qt::QueuedConnection);
  connect(this, &NodeAdapter::deinitNodeSignal, m_nodeInitializer, &InProcessNodeInitializer::stop, Qt::QueuedConnection);
}

NodeAdapter::~NodeAdapter() {
}

quintptr NodeAdapter::getPeerCount() const {
  Q_ASSERT(m_node != nullptr);
  return m_node->getPeerCount();
}

std::string NodeAdapter::convertPaymentId(const QString& _paymentIdString) const {
  Q_CHECK_PTR(m_node);
  try {
    return m_node->convertPaymentId(_paymentIdString.toStdString());
  } catch (std::runtime_error&) {
  }
  return std::string();
}

bool NodeAdapter::extractExtra(const std::string& _extra, std::vector<DynexCN::TransactionExtraField>& _fields) const {
  Q_CHECK_PTR(m_node);
  return m_node->extractExtra(_extra, _fields);
}

QString NodeAdapter::extractPaymentId(const std::vector<DynexCN::TransactionExtraField>& _fields) const {
  Q_CHECK_PTR(m_node);
  if (!_fields.size()) return {};
  return QString::fromStdString(m_node->extractPaymentId(_fields));
}

QString NodeAdapter::extractFromAddress(const std::vector<DynexCN::TransactionExtraField>& _fields) const {
  Q_CHECK_PTR(m_node);
  if (!_fields.size()) return {};
  return QString::fromStdString(m_node->extractFromAddress(_fields));
}

std::vector<std::pair<std::string, int64_t>> NodeAdapter::extractToAddress(const std::vector<DynexCN::TransactionExtraField>& _fields) const {
  Q_CHECK_PTR(m_node);
  if (!_fields.size()) return {};
  return m_node->extractToAddress(_fields);
}

DynexCN::IWalletLegacy* NodeAdapter::createWallet() const {
  Q_CHECK_PTR(m_node);
  return m_node->createWallet();
}

bool NodeAdapter::init() {
  Q_ASSERT(m_node == nullptr);

  QString connection = Settings::instance().getConnection();

  if (connection.compare("embedded") == 0) {

    LoggerAdapter::instance().log("Starting embedded node...");
    m_node = nullptr;
    return initInProcessNode();

  } else if (connection.compare("local") == 0) {

    LoggerAdapter::instance().log("Connecting to local node..."); 
    QUrl localNodeUrl = QUrl::fromUserInput(QString("127.0.0.1:%1").arg(Settings::instance().getLocalDaemonPort()));
    return initRPCNode(localNodeUrl);

  } else if(connection.compare("remote") == 0) {

    LoggerAdapter::instance().log("Connecting to remote node...");
    QUrl remoteNodeUrl = QUrl::fromUserInput(Settings::instance().getRemoteNode());
    return initRPCNode(remoteNodeUrl);

  } else {
	
    bool try_local = true;
    QString localurl = QString("127.0.0.1:%1").arg(DynexCN::RPC_DEFAULT_PORT);

    CURL *curl = curl_easy_init();
    if (curl) {
	  std::string readBuffer;
      long http_code = 0;
      QString url = QString("http://%1/%2").arg(localurl).arg("getinfo");

      curl_easy_setopt(curl, CURLOPT_URL, qPrintable(url));
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
      CURLcode res = curl_easy_perform(curl);

      if (res != CURLE_OK || curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code) != CURLE_OK || http_code != 200) {
        try_local = false;
        // try with custom port
        if (Settings::instance().getLocalDaemonPort() != DynexCN::RPC_DEFAULT_PORT) {
          localurl = QString("127.0.0.1:%1").arg(Settings::instance().getLocalDaemonPort());
          curl_easy_setopt(curl, CURLOPT_URL, qPrintable(url));
          res = curl_easy_perform(curl);
          if (res == CURLE_OK && curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code) == CURLE_OK && http_code == 200) {
            try_local = true;
          }
        }
      }
      curl_easy_cleanup(curl);
   	}

    if (try_local) {
      QUrl localNodeUrl = QUrl::fromUserInput(localurl);
      LoggerAdapter::instance().log("Connecting to local node...");
      if (initRPCNode(localNodeUrl)) return true;
    }

    LoggerAdapter::instance().log("Starting embedded node...");
    return initInProcessNode();
  }

  return false;
}

bool NodeAdapter::initRPCNode(QUrl nodeUrl) {
  Q_ASSERT(m_node == nullptr);

  m_node = createRpcNode(CurrencyAdapter::instance().getCurrency(), *this, nodeUrl.host().toStdString(), nodeUrl.port(DynexCN::RPC_DEFAULT_PORT));

  QTimer initTimer;
  initTimer.setInterval(3000);
  initTimer.setSingleShot(true);
  initTimer.start();
  m_node->init([](std::error_code _err) {
      Q_UNUSED(_err);
    });
  QEventLoop waitLoop;
  connect(&initTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
  connect(this, &NodeAdapter::peerCountUpdatedSignal, &waitLoop, &QEventLoop::quit);
  connect(this, &NodeAdapter::localBlockchainUpdatedSignal, &waitLoop, &QEventLoop::quit);
  waitLoop.exec();
  if (initTimer.isActive()) {
    initTimer.stop();
    Q_EMIT nodeInitCompletedSignal();
    return true;
  }

  delete m_node;
  m_node = nullptr;
  return false;
}

quint32 NodeAdapter::getLastKnownBlockHeight() const {
  Q_CHECK_PTR(m_node);
  return m_node->getLastKnownBlockHeight();
}

quint32 NodeAdapter::getLastLocalBlockHeight() const {
  Q_CHECK_PTR(m_node);
  return m_node->getLastLocalBlockHeight();
}

QDateTime NodeAdapter::getLastLocalBlockTimestamp() const {
  Q_CHECK_PTR(m_node);
  return QDateTime::fromSecsSinceEpoch(m_node->getLastLocalBlockTimestamp(), Qt::UTC);
}

quint64 NodeAdapter::getMinimalFee() const {
  Q_CHECK_PTR(m_node);
  return m_node->getMinimalFee();
}

void NodeAdapter::peerCountUpdated(Node& _node, size_t _count) {
  Q_UNUSED(_node);
  Q_EMIT peerCountUpdatedSignal(_count);
}

void NodeAdapter::localBlockchainUpdated(Node& _node, uint32_t _height) {
  Q_UNUSED(_node);
  Q_EMIT localBlockchainUpdatedSignal(_height);
}

void NodeAdapter::lastKnownBlockHeightUpdated(Node& _node, uint32_t _height) {
  Q_UNUSED(_node);
  Q_EMIT lastKnownBlockHeightUpdatedSignal(_height);
}

bool NodeAdapter::initInProcessNode() {
  Q_ASSERT(m_node == nullptr);
  m_nodeInitializerThread.start();
  DynexCN::CoreConfig coreConfig = makeCoreConfig();
  DynexCN::NetNodeConfig netNodeConfig = makeNetNodeConfig();
  Q_EMIT initNodeSignal(&m_node, this, coreConfig, netNodeConfig);
  QEventLoop waitLoop;
  connect(m_nodeInitializer, &InProcessNodeInitializer::nodeInitCompletedSignal, &waitLoop, &QEventLoop::quit);
  connect(m_nodeInitializer, &InProcessNodeInitializer::nodeInitFailedSignal, &waitLoop, &QEventLoop::exit);
  if (waitLoop.exec() != 0) {
    return false;
  }

  Q_EMIT localBlockchainUpdatedSignal(getLastLocalBlockHeight());
  Q_EMIT lastKnownBlockHeightUpdatedSignal(getLastKnownBlockHeight());
  return true;
}

void NodeAdapter::deinit() {
  if (m_node != nullptr) {
    if (m_nodeInitializerThread.isRunning()) {
      m_nodeInitializer->stop(&m_node);
      QEventLoop waitLoop;
      connect(m_nodeInitializer, &InProcessNodeInitializer::nodeDeinitCompletedSignal, &waitLoop, &QEventLoop::quit, Qt::QueuedConnection);
      waitLoop.exec();
      m_nodeInitializerThread.quit();
      m_nodeInitializerThread.wait();
    } else {
      //delete m_node;
      QFuture<void> f = QtConcurrent::run([=] { delete m_node; });
      while (f.isRunning()) {
        QCoreApplication::processEvents();
        QThread::msleep(30);
      }
      m_node = nullptr;
    }
  }
}

DynexCN::CoreConfig NodeAdapter::makeCoreConfig() const {
  DynexCN::CoreConfig config;
  boost::program_options::variables_map options;
  boost::any dataDir = Settings::instance().getDataDir().absolutePath().toStdString();
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  config.init(options);
  return config;
}

DynexCN::NetNodeConfig NodeAdapter::makeNetNodeConfig() const {
  DynexCN::NetNodeConfig config;
  boost::program_options::variables_map options;
  boost::any p2pBindIp = Settings::instance().getP2pBindIp().toStdString();
  boost::any p2pBindPort = static_cast<uint16_t>(Settings::instance().getP2pBindPort());
  boost::any p2pExternalPort = static_cast<uint16_t>(Settings::instance().getP2pExternalPort());
  boost::any p2pAllowLocalIp = Settings::instance().hasAllowLocalIpOption();
  boost::any dataDir = Settings::instance().getDataDir().absolutePath().toStdString();
  boost::any hideMyPort = Settings::instance().hasHideMyPortOption();
  options.insert(std::make_pair("p2p-bind-ip", boost::program_options::variable_value(p2pBindIp, false)));
  options.insert(std::make_pair("p2p-bind-port", boost::program_options::variable_value(p2pBindPort, false)));
  options.insert(std::make_pair("p2p-external-port", boost::program_options::variable_value(p2pExternalPort, false)));
  options.insert(std::make_pair("allow-local-ip", boost::program_options::variable_value(p2pAllowLocalIp, false)));
  std::vector<std::string> peerList = convertStringListToVector(Settings::instance().getPeers());
  if (!peerList.empty()) {
    options.insert(std::make_pair("add-peer", boost::program_options::variable_value(peerList, false)));
  }

  std::vector<std::string> priorityNodeList = convertStringListToVector(Settings::instance().getPriorityNodes());
  if (!priorityNodeList.empty()) {
    options.insert(std::make_pair("add-priority-node", boost::program_options::variable_value(priorityNodeList, false)));
  }

  std::vector<std::string> exclusiveNodeList = convertStringListToVector(Settings::instance().getExclusiveNodes());
  if (!exclusiveNodeList.empty()) {
    options.insert(std::make_pair("add-exclusive-node", boost::program_options::variable_value(exclusiveNodeList, false)));
  }

  std::vector<std::string> seedNodeList = convertStringListToVector(Settings::instance().getSeedNodes());
  if (!seedNodeList.empty()) {
    options.insert(std::make_pair("seed-node", boost::program_options::variable_value(seedNodeList, false)));
  }

  options.insert(std::make_pair("hide-my-port", boost::program_options::variable_value(hideMyPort, false)));
  options.insert(std::make_pair("data-dir", boost::program_options::variable_value(dataDir, false)));
  config.init(options);
  config.setTestnet(Settings::instance().isTestnet());
  return config;
}

}

#include "NodeAdapter.moc"
