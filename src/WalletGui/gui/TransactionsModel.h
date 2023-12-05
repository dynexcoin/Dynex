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

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <IWalletLegacy.h>

namespace WalletGui {

typedef QPair<DynexCN::TransactionId, DynexCN::TransferId> TransactionTransferId;

class TransactionsModel : public QAbstractItemModel {
  Q_OBJECT
  Q_ENUMS(Columns)
  Q_ENUMS(Roles)

public:
  enum Columns{
    COLUMN_STATE = 0, COLUMN_DATE, COLUMN_AMOUNT, COLUMN_ADDRESS, COLUMN_PAYMENT_ID, COLUMN_HASH, COLUMN_FEE,
    COLUMN_HEIGHT, COLUMN_TYPE
  };

  enum Roles{
    ROLE_DATE = Qt::UserRole, ROLE_TYPE, ROLE_HASH, ROLE_ADDRESS, ROLE_AMOUNT, ROLE_PAYMENT_ID, ROLE_ICON,
    ROLE_TRANSACTION_ID, ROLE_HEIGHT, ROLE_FEE, ROLE_NUMBER_OF_CONFIRMATIONS, ROLE_COLUMN, ROLE_ROW, ROLE_STATE
  };

  static TransactionsModel& instance();

  int columnCount(const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int rowCount(const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant headerData(int _section, Qt::Orientation _orientation, int _role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QModelIndex index(int _row, int _column, const QModelIndex& _parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QModelIndex	parent(const QModelIndex& _index) const Q_DECL_OVERRIDE;

  QByteArray toCsv() const;

private:
  QVector<TransactionTransferId> m_transfers;
  QHash<DynexCN::TransactionId, QPair<quint32, quint32> > m_transactionRow;

  TransactionsModel();
  ~TransactionsModel();

  QVariant getDisplayRole(const QModelIndex& _index) const;
  QVariant getDecorationRole(const QModelIndex& _index) const;
  QVariant getAlignmentRole(const QModelIndex& _index) const;
  QVariant getUserRole(const QModelIndex& _index, int _role, DynexCN::TransactionId _transactionId, DynexCN::WalletLegacyTransaction& _transaction,
    DynexCN::TransferId _transferId, DynexCN::WalletLegacyTransfer& _transfer) const;

  void reloadWalletTransactions();
  void appendTransaction(DynexCN::TransactionId _id, quint32& _row_count);
  void appendTransaction(DynexCN::TransactionId _id);
  void updateWalletTransaction(DynexCN::TransactionId _id);
  void localBlockchainUpdated(quint64 _height);
  void reset();
};

}
