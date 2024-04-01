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

#include <QMutex>
#include <QObject>
#include <QTimer>

#include <atomic>
#include <fstream>

#include <IWalletLegacy.h>

namespace WalletGui {

class WalletAdapter : public QObject, public DynexCN::IWalletLegacyObserver {
  Q_OBJECT
  Q_DISABLE_COPY(WalletAdapter)

public:
  static WalletAdapter& instance();

  void open(const QString& _password);
  void createWallet();
  void createNonDeterministic();
  void createWithKeys(const DynexCN::AccountKeys& _keys);
  void createWithKeys(const DynexCN::AccountKeys& _keys, const quint32 _sync_heigth);
  void close();
  bool save(bool _details, bool _cache);
  void backup(const QString& _file);
  void autoBackup();
  void reset();

  QString getAddress() const;
  quint64 getActualBalance() const;
  quint64 getPendingBalance() const;
  quint64 getTransactionCount() const;
  quint64 getTransferCount() const;
  bool getTransaction(DynexCN::TransactionId& _id, DynexCN::WalletLegacyTransaction& _transaction);
  bool getTransfer(DynexCN::TransferId& _id, DynexCN::WalletLegacyTransfer& _transfer);
  bool getAccountKeys(DynexCN::AccountKeys& _keys);
  bool isOpen() const;
  void sendTransaction(const QVector<DynexCN::WalletLegacyTransfer>& _transfers, quint64 _fee, const QString& _payment_id, quint64 _mixin);

  bool changePassword(const QString& _old_pass, const QString& _new_pass);
  void setWalletFile(const QString& _path);

  void initCompleted(std::error_code _result) Q_DECL_OVERRIDE;
  void saveCompleted(std::error_code _result) Q_DECL_OVERRIDE;
  void synchronizationProgressUpdated(uint32_t _current, uint32_t _total) Q_DECL_OVERRIDE;
  void synchronizationCompleted(std::error_code _error) Q_DECL_OVERRIDE;
  void actualBalanceUpdated(uint64_t _actual_balance) Q_DECL_OVERRIDE;
  void pendingBalanceUpdated(uint64_t _pending_balance) Q_DECL_OVERRIDE;
  void externalTransactionCreated(DynexCN::TransactionId _transaction_id) Q_DECL_OVERRIDE;
  void sendTransactionCompleted(DynexCN::TransactionId _transaction_id, std::error_code _result) Q_DECL_OVERRIDE;
  void transactionUpdated(DynexCN::TransactionId _transaction_id) Q_DECL_OVERRIDE;

  bool isDeterministic() const;
  bool isDeterministic(DynexCN::AccountKeys& _keys) const;
  QString getMnemonicSeed(QString _language) const;
  DynexCN::AccountKeys getKeysFromMnemonicSeed(QString& _seed) const;

  quint64 estimateFusion(quint64 _threshold);
  std::list<DynexCN::TransactionOutputInformation> getFusionTransfersToSend(quint64 _threshold, size_t _min_input_count, size_t _max_input_count);
  void sendFusionTransaction(const std::list<DynexCN::TransactionOutputInformation>& _fusion_inputs, quint64 _mixin);
  bool isFusionTransaction(const DynexCN::WalletLegacyTransaction& walletTx) const;
  void optimize(const quint64 mixin);
  QString getReserveProof(const quint64 _reserve);

private:
  std::fstream m_file;
  DynexCN::IWalletLegacy* m_wallet;
  QMutex m_mutex;
  std::atomic<bool> m_opened;
  std::atomic<bool> m_isBackupInProgress;
  std::atomic<bool> m_isSynchronized;
  std::atomic<quint64> m_lastWalletTransactionId;
  QTimer m_newTransactionsNotificationTimer;
  QTimer m_statusTimer;

  WalletAdapter();
  ~WalletAdapter();

  void onWalletInitCompleted(int _error, const QString& _error_text);
  void onWalletSendTransactionCompleted(DynexCN::TransactionId _transaction_id, int _error, const QString& _error_text);

  bool importLegacyWallet(const QString& _password);
  bool save(const QString& _file, bool _details, bool _cache);
  void lock();
  void unlock();
  bool openFile(const QString& _file, bool _read_only);
  void closeFile();
  void notifyAboutLastTransaction();

  static void renameFile(const QString& _old_name, const QString& _new_name);
  Q_SLOT void updateBlockStatusText();
  Q_SLOT void updateBlockStatusTextWithDelay();

Q_SIGNALS:
  void walletInitCompletedSignal(int _error, const QString& _error_text);
  void walletCloseCompletedSignal();
  void walletSaveCompletedSignal(int _error, const QString& _error_text);
  void walletSynchronizationProgressUpdatedSignal(quint64 _current, quint64 _total);
  void walletSynchronizationCompletedSignal(int _error, const QString& _error_text);
  void walletActualBalanceUpdatedSignal(quint64 _actual_balance);
  void walletPendingBalanceUpdatedSignal(quint64 _pending_balance);
  void walletTransactionCreatedSignal(DynexCN::TransactionId _transaction_id);
  void walletSendTransactionCompletedSignal(DynexCN::TransactionId _transaction_id, int _error, const QString& _error_text);
  void walletTransactionUpdatedSignal(DynexCN::TransactionId _transaction_id);
  void walletStateChangedSignal(const QString& _state_text);

  void openWalletWithPasswordSignal(bool _error);
  void changeWalletPasswordSignal();
  void updateWalletAddressSignal(const QString& _address);
  void reloadWalletTransactionsSignal();
  void updateBlockStatusTextSignal();
  void updateBlockStatusTextWithDelaySignal();
};



}
