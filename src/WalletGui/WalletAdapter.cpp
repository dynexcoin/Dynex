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
#include <QMessageBox>
#include <QDateTime>
#include <QLocale>
#include <QVector>

#include <Common/Util.h>
#include <Wallet/WalletErrors.h>
#include <Wallet/LegacyKeysImporter.h>

#include "NodeAdapter.h"
#include "Settings.h"
#include "WalletAdapter.h"
#include "CurrencyAdapter.h"
#include "DynexCNConfig.h"
#include "crypto/crypto.h"
#include "Common/Base58.h"
#include "DynexCNCore/DynexCNBasic.h"
#include "Mnemonics/electrum-words.h"
#include "gui/VerifyMnemonicSeedDialog.h"

extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}

#define GENERATE_DETERMINISTIC

namespace WalletGui {

const quint32 MSECS_IN_HOUR = 60 * 60 * 1000;
const quint32 MSECS_IN_MINUTE = 60 * 1000;

const quint32 LAST_BLOCK_INFO_UPDATING_INTERVAL = 1 * MSECS_IN_MINUTE;
const quint32 LAST_BLOCK_INFO_WARNING_INTERVAL = 1 * MSECS_IN_HOUR;

WalletAdapter& WalletAdapter::instance() {
  static WalletAdapter inst;
  return inst;
}

WalletAdapter::WalletAdapter() : QObject(), m_wallet(nullptr), m_mutex(), m_isBackupInProgress(false),
  m_isSynchronized(false), m_newTransactionsNotificationTimer(), m_statusTimer(),
  m_lastWalletTransactionId(std::numeric_limits<quint64>::max()) {
  connect(this, &WalletAdapter::walletInitCompletedSignal, this, &WalletAdapter::onWalletInitCompleted, Qt::QueuedConnection);
  connect(this, &WalletAdapter::walletSendTransactionCompletedSignal, this, &WalletAdapter::onWalletSendTransactionCompleted, Qt::QueuedConnection);
  connect(this, &WalletAdapter::updateBlockStatusTextSignal, this, &WalletAdapter::updateBlockStatusText, Qt::QueuedConnection);
  connect(this, &WalletAdapter::updateBlockStatusTextWithDelaySignal, this, &WalletAdapter::updateBlockStatusTextWithDelay, Qt::QueuedConnection);
  connect(&m_newTransactionsNotificationTimer, &QTimer::timeout, this, &WalletAdapter::notifyAboutLastTransaction);
  connect(&m_statusTimer, &QTimer::timeout, this, &WalletAdapter::updateBlockStatusText);
  connect(this, &WalletAdapter::walletSynchronizationProgressUpdatedSignal, this, [&]() {
    if (!m_newTransactionsNotificationTimer.isActive()) {
      m_newTransactionsNotificationTimer.start();
    }
  }, Qt::QueuedConnection);

  connect(this, &WalletAdapter::walletSynchronizationCompletedSignal, this, [&]() {
    m_newTransactionsNotificationTimer.stop();
    notifyAboutLastTransaction();
  }, Qt::QueuedConnection);

  m_newTransactionsNotificationTimer.setInterval(500);
  m_statusTimer.setSingleShot(true);
}

WalletAdapter::~WalletAdapter() {
}

QString WalletAdapter::getAddress() const {
  try {
    return m_wallet == nullptr ? QString() : QString::fromStdString(m_wallet->getAddress());
  } catch (std::system_error&) {
    return QString();
  }
}

quint64 WalletAdapter::getActualBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->actualBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

quint64 WalletAdapter::getPendingBalance() const {
  try {
    return m_wallet == nullptr ? 0 : m_wallet->pendingBalance();
  } catch (std::system_error&) {
    return 0;
  }
}

void WalletAdapter::open(const QString& _password) {
  Q_ASSERT(m_wallet == nullptr);
  Settings::instance().setEncrypted(!_password.isEmpty());
  QString wallet = Settings::instance().getWalletFile();
  QString msg = "Opening wallet " + wallet;
  Q_EMIT walletStateChangedSignal(msg);

  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);

  if (QFile::exists(wallet)) {
    if (Settings::instance().getWalletFile().endsWith(".keys")) {
      if(!importLegacyWallet(_password)) {
        return;
      }
    }

    if (openFile(wallet, true)) {
      try {
        m_wallet->initAndLoad(m_file, _password.toStdString());
      } catch (std::system_error&) {
        closeFile();
        delete m_wallet;
        m_wallet = nullptr;
      }
    }
  } else {
    QMessageBox::critical(nullptr, tr("Wallet file not found"), wallet, QMessageBox::Ok);
  }
}

void WalletAdapter::createWallet() {
  Q_ASSERT(m_wallet == nullptr);
  Settings::instance().setEncrypted(false);
  Q_EMIT walletStateChangedSignal(tr("Creating wallet"));
  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);

  try {
    m_wallet->initAndGenerateDeterministic("");
    VerifyMnemonicSeedDialog dlg(nullptr);
    dlg.exec();
  } catch (std::system_error&) {
    delete m_wallet;
    m_wallet = nullptr;
  }
}

void WalletAdapter::createNonDeterministic() {
  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);
  Settings::instance().setEncrypted(false);
  try {
    m_wallet->initAndGenerate("");
  } catch (std::system_error&) {
    delete m_wallet;
    m_wallet = nullptr;
  }
}

void WalletAdapter::createWithKeys(const DynexCN::AccountKeys& _keys) {
  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);
  Settings::instance().setEncrypted(false);
  Q_EMIT walletStateChangedSignal(tr("Creating wallet"));
  m_wallet->initWithKeys(_keys, "");
}

void WalletAdapter::createWithKeys(const DynexCN::AccountKeys& _keys, const quint32 _sync_heigth) {
  m_wallet = NodeAdapter::instance().createWallet();
  m_wallet->addObserver(this);
  Settings::instance().setEncrypted(false);
  Q_EMIT walletStateChangedSignal(tr("Creating wallet"));
  m_wallet->initWithKeys(_keys, "", _sync_heigth);
}

bool WalletAdapter::isOpen() const {
  return m_wallet != nullptr;
}

bool WalletAdapter::importLegacyWallet(const QString &_password) {
  QString fileName = Settings::instance().getWalletFile();
  Settings::instance().setEncrypted(!_password.isEmpty());
  try {
    fileName.replace(fileName.lastIndexOf(".keys"), 5, ".wallet");
    if (!openFile(fileName, false)) {
      delete m_wallet;
      m_wallet = nullptr;
      return false;
    }

    DynexCN::importLegacyKeys(Settings::instance().getWalletFile().toStdString(), _password.toStdString(), m_file);
    closeFile();
    Settings::instance().setWalletFile(fileName);
    return true;
  } catch (std::system_error& _err) {
    closeFile();
    if (_err.code().value() == DynexCN::error::WRONG_PASSWORD) {
      Settings::instance().setEncrypted(true);
      Q_EMIT openWalletWithPasswordSignal(!_password.isEmpty());
    }
  } catch (std::runtime_error& _err) {
    closeFile();
  }

  delete m_wallet;
  m_wallet = nullptr;
  return false;
}

void WalletAdapter::close() {
  Q_CHECK_PTR(m_wallet);
  save(true, true);
  lock();
  m_wallet->removeObserver(this);
  m_isSynchronized = false;
  m_newTransactionsNotificationTimer.stop();
  m_statusTimer.stop();
  m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  Q_EMIT walletCloseCompletedSignal();
  QCoreApplication::processEvents();
  delete m_wallet;
  m_wallet = nullptr;
  unlock();
}

bool WalletAdapter::save(bool _details, bool _cache) {
  return save(Settings::instance().getWalletFile() + ".temp", _details, _cache);
}

bool WalletAdapter::save(const QString& _file, bool _details, bool _cache) {
  Q_CHECK_PTR(m_wallet);
  if (openFile(_file, false)) {
    Q_EMIT walletStateChangedSignal(tr("Saving data"));
    try {
      m_wallet->save(m_file, _details, _cache);
    } catch (std::system_error&) {
      closeFile();
      return false;
    }

  } else {
    return false;
  }

  return true;
}

void WalletAdapter::backup(const QString& _file) {
  if (save(_file.endsWith(".wallet") ? _file : _file + ".wallet", true, false)) {
    m_isBackupInProgress = true;
  }
}

void WalletAdapter::autoBackup(){
  QString source = Settings::instance().getWalletFile();
  source.append(QString(".backup"));

  if (!source.isEmpty() && !QFile::exists(source)) {
    if (save(source, true, false)) {
      m_isBackupInProgress = true;
    }
  }
}

void WalletAdapter::reset() {
  Q_CHECK_PTR(m_wallet);
  save(false, false);
  lock();
  m_wallet->removeObserver(this);
  m_isSynchronized = false;
  m_newTransactionsNotificationTimer.stop();
  m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  Q_EMIT walletCloseCompletedSignal();
  QCoreApplication::processEvents();
  delete m_wallet;
  m_wallet = nullptr;
  unlock();
}


quint64 WalletAdapter::getTransactionCount() const {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->getTransactionCount();
  } catch (std::system_error&) {
  }

  return 0;
}

quint64 WalletAdapter::getTransferCount() const {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->getTransferCount();
  } catch (std::system_error&) {
  }

  return 0;
}

bool WalletAdapter::getTransaction(DynexCN::TransactionId& _id, DynexCN::WalletLegacyTransaction& _transaction) {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->getTransaction(_id, _transaction);
  } catch (std::system_error&) {
  }

  return false;
}

bool WalletAdapter::getTransfer(DynexCN::TransferId& _id, DynexCN::WalletLegacyTransfer& _transfer) {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->getTransfer(_id, _transfer);
  } catch (std::system_error&) {
  }

  return false;
}

bool WalletAdapter::getAccountKeys(DynexCN::AccountKeys& _keys) {
  Q_CHECK_PTR(m_wallet);
  try {
    m_wallet->getAccountKeys(_keys);
    return true;
  } catch (std::system_error&) {
  }

  return false;
}

void WalletAdapter::sendTransaction(const QVector<DynexCN::WalletLegacyTransfer>& _transfers, quint64 _fee, const QString& _paymentId, quint64 _mixin) {
  Q_CHECK_PTR(m_wallet);
  if (!m_isSynchronized) {
    QMessageBox::warning(nullptr, tr("Warning"), tr("Wallet is not synchronized!"), QMessageBox::Ok);
    return;
  }  
  try {
    lock();
    std::vector<DynexCN::WalletLegacyTransfer> vec(_transfers.begin(), _transfers.end());
    m_wallet->sendTransaction(vec, _fee, NodeAdapter::instance().convertPaymentId(_paymentId), _mixin, 0);
    Q_EMIT walletStateChangedSignal(tr("Sending transaction"));
  } catch (std::system_error&) {
    unlock();
  }
}

bool WalletAdapter::changePassword(const QString& _oldPassword, const QString& _newPassword) {
  Q_CHECK_PTR(m_wallet);
  try {
    if (m_wallet->changePassword(_oldPassword.toStdString(), _newPassword.toStdString()).value() == DynexCN::error::WRONG_PASSWORD) {
      return false;
    }
  } catch (std::system_error&) {
    return false;
  }

  Settings::instance().setEncrypted(!_newPassword.isEmpty());
  return save(true, true);
}

void WalletAdapter::setWalletFile(const QString& _path) {
  Q_ASSERT(m_wallet == nullptr);
  Settings::instance().setWalletFile(_path);
}

void WalletAdapter::initCompleted(std::error_code _error) {
  if (m_file.is_open()) {
    closeFile();
  }

  Q_EMIT walletInitCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
}

void WalletAdapter::onWalletInitCompleted(int _error, const QString& _errorText) {
  switch(_error) {
  case 0: {
    Q_EMIT walletActualBalanceUpdatedSignal(m_wallet->actualBalance());
    Q_EMIT walletPendingBalanceUpdatedSignal(m_wallet->pendingBalance());
    Q_EMIT updateWalletAddressSignal(QString::fromStdString(m_wallet->getAddress()));
    Q_EMIT reloadWalletTransactionsSignal();
    Q_EMIT walletStateChangedSignal(tr("Ready"));
    Q_EMIT updateBlockStatusTextWithDelay();
    if (!QFile::exists(Settings::instance().getWalletFile())) {
      save(true, true);
    }

    break;
  }
  case DynexCN::error::WRONG_PASSWORD:
    Q_EMIT openWalletWithPasswordSignal(Settings::instance().isEncrypted());
    Settings::instance().setEncrypted(true);
    delete m_wallet;
    m_wallet = nullptr;
    break;
  default: {
    delete m_wallet;
    m_wallet = nullptr;
    break;
  }
  }
}

void WalletAdapter::saveCompleted(std::error_code _error) {
  if (!_error && !m_isBackupInProgress) {
    closeFile();
    renameFile(Settings::instance().getWalletFile() + ".temp", Settings::instance().getWalletFile());
    Q_EMIT walletStateChangedSignal(tr("Ready"));
    Q_EMIT updateBlockStatusTextWithDelaySignal();
  } else if (m_isBackupInProgress) {
    m_isBackupInProgress = false;
    closeFile();
  } else {
    closeFile();
  }

  Q_EMIT walletSaveCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
}

void WalletAdapter::synchronizationProgressUpdated(uint32_t _current, uint32_t _total) {
  m_isSynchronized = false;
  Q_EMIT updateBlockStatusTextWithDelaySignal();
  Q_EMIT walletStateChangedSignal(QString("%1 %2/%3").arg(tr("Synchronizing")).arg(_current).arg(_total));
  Q_EMIT walletSynchronizationProgressUpdatedSignal(_current, _total);
}

void WalletAdapter::synchronizationCompleted(std::error_code _error) {
  if (!_error) {
    m_isSynchronized = true;
    //Q_EMIT updateBlockStatusTextSignal();
    Q_EMIT updateBlockStatusTextWithDelaySignal();
    Q_EMIT walletSynchronizationCompletedSignal(_error.value(), QString::fromStdString(_error.message()));
  }
}

void WalletAdapter::actualBalanceUpdated(uint64_t _actual_balance) {
  Q_EMIT walletActualBalanceUpdatedSignal(_actual_balance);
}

void WalletAdapter::pendingBalanceUpdated(uint64_t _pending_balance) {
  Q_EMIT walletPendingBalanceUpdatedSignal(_pending_balance);
}

void WalletAdapter::externalTransactionCreated(DynexCN::TransactionId _transactionId) {
  if (!m_isSynchronized) {
    m_lastWalletTransactionId = _transactionId;
  } else {
    Q_EMIT walletTransactionCreatedSignal(_transactionId);
  }
}

void WalletAdapter::sendTransactionCompleted(DynexCN::TransactionId _transaction_id, std::error_code _error) {
  unlock();
  Q_EMIT walletSendTransactionCompletedSignal(_transaction_id, _error.value(), QString::fromStdString(_error.message()));
  Q_EMIT updateBlockStatusTextWithDelaySignal();
}

void WalletAdapter::onWalletSendTransactionCompleted(DynexCN::TransactionId _transactionId, int _error, const QString& _errorText) {
  if (_error) {
    return;
  }

  DynexCN::WalletLegacyTransaction transaction;
  if (!this->getTransaction(_transactionId, transaction)) {
    return;
  }

  if (transaction.transferCount == 0) {
    return;
  }

  Q_EMIT walletTransactionCreatedSignal(_transactionId);

  save(true, true);
}

void WalletAdapter::transactionUpdated(DynexCN::TransactionId _transactionId) {
  Q_EMIT walletTransactionUpdatedSignal(_transactionId);
}

void WalletAdapter::lock() {
  m_mutex.lock();
}

void WalletAdapter::unlock() {
  m_mutex.unlock();
}

bool WalletAdapter::openFile(const QString& _file, bool _readOnly) {
  lock();
  m_file.open(_file.toStdString(), std::ios::binary | (_readOnly ? std::ios::in : (std::ios::out | std::ios::trunc)));
  if (!m_file.is_open()) {
    unlock();
  }

  return m_file.is_open();
}

void WalletAdapter::closeFile() {
  m_file.close();
  unlock();
}

void WalletAdapter::notifyAboutLastTransaction() {
  if (m_lastWalletTransactionId != std::numeric_limits<quint64>::max()) {
    Q_EMIT walletTransactionCreatedSignal(m_lastWalletTransactionId);
    m_lastWalletTransactionId = std::numeric_limits<quint64>::max();
  }
}

void WalletAdapter::renameFile(const QString& _oldName, const QString& _newName) {
  Q_ASSERT(QFile::exists(_oldName));
  QFile::remove(_newName);
  QFile::rename(_oldName, _newName);
}

void WalletAdapter::updateBlockStatusText() {
  if (m_wallet == nullptr) {
    return;
  }

  const QDateTime currentTime = QDateTime::currentDateTimeUtc();
  const QDateTime blockTime = NodeAdapter::instance().getLastLocalBlockTimestamp();
  quint64 blockAge = blockTime.msecsTo(currentTime);
  const QString warningString = blockTime.msecsTo(currentTime) < LAST_BLOCK_INFO_WARNING_INTERVAL ? "" :
    QString("  Warning: last block was received %1 hours %2 minutes ago").arg(blockAge / MSECS_IN_HOUR).arg(blockAge % MSECS_IN_HOUR / MSECS_IN_MINUTE);
  Q_EMIT walletStateChangedSignal(QString(tr("Wallet synchronized. Height: %1  |  Time (UTC): %2%3")).
    arg(NodeAdapter::instance().getLastLocalBlockHeight()).
    arg(QLocale(QLocale::English).toString(blockTime, "dd MMM yyyy, HH:mm:ss")).
    arg(warningString));

  Q_EMIT updateBlockStatusTextWithDelay();
}

void WalletAdapter::updateBlockStatusTextWithDelay() {
  m_statusTimer.stop();
  m_statusTimer.start(5000);
}

bool WalletAdapter::isDeterministic() const {
  Crypto::SecretKey second;
  DynexCN::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  keccak((uint8_t *)&keys.spendSecretKey, sizeof(Crypto::SecretKey), (uint8_t *)&second, sizeof(Crypto::SecretKey));
  sc_reduce32((uint8_t *)&second);
  bool keys_deterministic = memcmp(second.data,keys.viewSecretKey.data, sizeof(Crypto::SecretKey)) == 0;
  return keys_deterministic;
}

bool WalletAdapter::isDeterministic(DynexCN::AccountKeys& _keys) const {
  Crypto::SecretKey second;
  WalletAdapter::instance().getAccountKeys(_keys);
  keccak((uint8_t *)&_keys.spendSecretKey, sizeof(Crypto::SecretKey), (uint8_t *)&second, sizeof(Crypto::SecretKey));
  sc_reduce32((uint8_t *)&second);
  bool keys_deterministic = memcmp(second.data,_keys.viewSecretKey.data, sizeof(Crypto::SecretKey)) == 0;
  return keys_deterministic;
}

QString WalletAdapter::getMnemonicSeed(QString _language) const {
  std::string electrum_words;
  if(!WalletAdapter::instance().isDeterministic()) {
    return "Wallet is non-deterministic and has no seed";
  }
  DynexCN::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);
  std::string seed_language = _language.toUtf8().constData();
  Crypto::ElectrumWords::bytes_to_words(keys.spendSecretKey, electrum_words, seed_language);
  return QString::fromStdString(electrum_words);
}

DynexCN::AccountKeys WalletAdapter::getKeysFromMnemonicSeed(QString& _seed) const {
  DynexCN::AccountKeys keys;
  std::string m_seed_language;
  if(!Crypto::ElectrumWords::words_to_bytes(_seed.toStdString(), keys.spendSecretKey, m_seed_language)) {
    QMessageBox::critical(nullptr, tr("Mnemonic seed is not correct"), tr("There must be an error in mnemonic seed. Make sure you entered it correctly."), QMessageBox::Ok);
  }
  Crypto::secret_key_to_public_key(keys.spendSecretKey,keys.address.spendPublicKey);
  Crypto::SecretKey second;
  keccak((uint8_t *)&keys.spendSecretKey, sizeof(Crypto::SecretKey), (uint8_t *)&second, sizeof(Crypto::SecretKey));
  Crypto::generate_deterministic_keys(keys.address.viewPublicKey,keys.viewSecretKey,second);
  return keys;
}

quint64 WalletAdapter::estimateFusion(quint64 _threshold) {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->estimateFusion(_threshold);
  } catch (std::system_error&) {
  }
  return 0;
}

std::list<DynexCN::TransactionOutputInformation> WalletAdapter::getFusionTransfersToSend(quint64 _threshold, size_t _min_input_count, size_t _max_input_count) {
  Q_CHECK_PTR(m_wallet);
  try {
    return m_wallet->selectFusionTransfersToSend(_threshold, _min_input_count, _max_input_count);
  } catch (std::system_error&) {
  }
  return {};
}

void WalletAdapter::sendFusionTransaction(const std::list<DynexCN::TransactionOutputInformation>& _fusion_inputs, quint64 _mixin) {
  Q_CHECK_PTR(m_wallet);
  try {
    lock();
    Q_EMIT walletStateChangedSignal(tr("Optimizing wallet"));
    m_wallet->sendFusionTransaction(_fusion_inputs, 0, "", _mixin, 0);
  } catch (std::system_error&) {
    unlock();
  }
}

bool WalletAdapter::isFusionTransaction(const DynexCN::WalletLegacyTransaction& walletTx) const {
  Q_CHECK_PTR(m_wallet);
  return m_wallet->isFusionTransaction(walletTx);
}

void WalletAdapter::optimize(const quint64 mixin) {
  if (!isOpen() || !m_isSynchronized) return;
  uint64_t fusionThreshold = getActualBalance();
  //size_t fusionReadyCount = estimateFusion(fusionThreshold);
  const size_t MAX_FUSION_OUTPUT_COUNT = 4;
  size_t estimatedFusionInputsCount = CurrencyAdapter::instance().getCurrency().getApproximateMaximumInputCount(CurrencyAdapter::instance().getCurrency().fusionTxMaxSize(), MAX_FUSION_OUTPUT_COUNT, mixin);
  if (estimatedFusionInputsCount < CurrencyAdapter::instance().getCurrency().fusionTxMinInputCount()) {
    QMessageBox::warning(nullptr, tr("Optimize"), tr("Anonimity level is too high!"), QMessageBox::Ok);
    return;
  }
  std::list<DynexCN::TransactionOutputInformation> fusionInputs = getFusionTransfersToSend(fusionThreshold, CurrencyAdapter::instance().getCurrency().fusionTxMinInputCount(), estimatedFusionInputsCount);
  if (fusionInputs.size() < CurrencyAdapter::instance().getCurrency().fusionTxMinInputCount()) {
    QMessageBox::warning(nullptr, tr("Optimize"), tr("Nothing to do!"), QMessageBox::Ok);
    return;
  }
  if (QMessageBox::warning(nullptr, tr("Optimize"), tr("Send fusion transaction?"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
    sendFusionTransaction(fusionInputs, mixin);
  }
}

}
