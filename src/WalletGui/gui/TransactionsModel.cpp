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

#include <QDateTime>
#include <QFont>
#include <QMetaEnum>
#include <QPixmap>
#include <QPixmapCache>

#include "CurrencyAdapter.h"
#include "NodeAdapter.h"
#include "TransactionsModel.h"
#include "WalletAdapter.h"
#include "AddressBookModel.h"

namespace WalletGui {

enum class TransactionType : quint8 {MINED, INPUT, OUTPUT, INOUT, FUSION};

enum class TransactionState : quint8 {ACTIVE, DELETED, SENDING, CANCELLED, FAILED};

namespace {

QPixmap getTransactionIcon(TransactionType _transactionType) {
  switch (_transactionType) {
  case TransactionType::MINED:
    return QPixmap(":icons/tx-mined");
  case TransactionType::INPUT:
    return QPixmap(":icons/tx-input");
  case TransactionType::OUTPUT:
    return QPixmap(":icons/tx-output");
  case TransactionType::INOUT:
  case TransactionType::FUSION:
    return QPixmap(":icons/tx-inout");
  default:
    break;
  }

  return QPixmap();
}

}

TransactionsModel& TransactionsModel::instance() {
  static TransactionsModel inst;
  return inst;
}

TransactionsModel::TransactionsModel() : QAbstractItemModel() {
  connect(&WalletAdapter::instance(), &WalletAdapter::reloadWalletTransactionsSignal, this, &TransactionsModel::reloadWalletTransactions,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionCreatedSignal, this,
    static_cast<void(TransactionsModel::*)(DynexCN::TransactionId)>(&TransactionsModel::appendTransaction), Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletTransactionUpdatedSignal, this, &TransactionsModel::updateWalletTransaction,
    Qt::QueuedConnection);
  connect(&NodeAdapter::instance(), &NodeAdapter::localBlockchainUpdatedSignal, this, &TransactionsModel::localBlockchainUpdated,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &TransactionsModel::reset,
    Qt::QueuedConnection);
}

TransactionsModel::~TransactionsModel() {
}

int TransactionsModel::columnCount(const QModelIndex& _parent) const {
  return TransactionsModel::staticMetaObject.enumerator(TransactionsModel::staticMetaObject.indexOfEnumerator("Columns")).keyCount();
}

int TransactionsModel::rowCount(const QModelIndex& _parent) const {
  return m_transfers.size();
}

QVariant TransactionsModel::headerData(int _section, Qt::Orientation _orientation, int _role) const {
  if(_orientation != Qt::Horizontal) {
    return QVariant();
  }

  switch(_role) {
  case Qt::DisplayRole:
    switch(_section) {
    case COLUMN_STATE:
      return QVariant();
    case COLUMN_DATE:
      return tr("Date");
    case COLUMN_TYPE:
      return tr("Type");
    case COLUMN_HASH:
      return tr("Hash");
    case COLUMN_ADDRESS:
      return tr("Address");
    case COLUMN_AMOUNT:
      return tr("Amount");
    case COLUMN_PAYMENT_ID:
      return tr("PaymentID");
    default:
      break;
    }
    break;

  case Qt::TextAlignmentRole:
    if (_section == COLUMN_AMOUNT) {
      return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    }
    return QVariant();

  case ROLE_COLUMN:
    return _section;
  }

  return QVariant();
}

QVariant TransactionsModel::data(const QModelIndex& _index, int _role) const {
  if(!_index.isValid()) {
    return QVariant();
  }

  DynexCN::WalletLegacyTransaction transaction;
  DynexCN::WalletLegacyTransfer transfer;
  DynexCN::TransactionId transactionId = m_transfers.value(_index.row()).first;
  DynexCN::TransferId transferId = m_transfers.value(_index.row()).second;

  if(!WalletAdapter::instance().getTransaction(transactionId, transaction) ||
    (m_transfers.value(_index.row()).second != DynexCN::WALLET_LEGACY_INVALID_TRANSFER_ID &&
    !WalletAdapter::instance().getTransfer(transferId, transfer))) {
    return QVariant();
  }

  switch(_role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return getDisplayRole(_index);

  case Qt::DecorationRole:
    return getDecorationRole(_index);

  case Qt::TextAlignmentRole:
    return getAlignmentRole(_index);

  default:
    return getUserRole(_index, _role, transactionId, transaction, transferId, transfer);
  }

  return QVariant();
}

QModelIndex TransactionsModel::index(int _row, int _column, const QModelIndex& _parent) const {
  if(_parent.isValid()) {
    return QModelIndex();
  }

  return createIndex(_row, _column, _row);
}

QModelIndex TransactionsModel::parent(const QModelIndex& _index) const {
  return QModelIndex();
}

QByteArray TransactionsModel::toCsv() const {
  QByteArray res;
  res.append("\"State\",\"Date\",\"Amount\",\"Fee\",\"Hash\",\"Height\",\"Address\",\"Payment ID\"\n");
  for (int row = 0; row < rowCount(); ++row) {
    QModelIndex ind = index(row, 0);
    res.append("\"").append(ind.data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_DATE).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_AMOUNT).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_FEE).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_HASH).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_HEIGHT).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_ADDRESS).data().toString().toUtf8()).append("\",");
    res.append("\"").append(ind.sibling(row, COLUMN_PAYMENT_ID).data().toString().toUtf8()).append("\"\n");
  }

  return res;
}

QVariant TransactionsModel::getDisplayRole(const QModelIndex& _index) const {
  switch(_index.column()) {
  case COLUMN_DATE: {
    QDateTime date = _index.data(ROLE_DATE).toDateTime();
    return (date.isNull() || !date.isValid() ? "-" : date.toString("dd-MM-yy HH:mm"));
  }

  case COLUMN_HASH:
    return _index.data(ROLE_HASH).toString(); //ByteArray().toHex().toUpper();

  case COLUMN_ADDRESS: {
    TransactionType transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());
    if (transactionType == TransactionType::INPUT || transactionType == TransactionType::MINED ||
        transactionType == TransactionType::INOUT || transactionType == TransactionType::FUSION) {
      return QString(tr("me (%1)").arg(WalletAdapter::instance().getAddress()));
    } 

    QString transactionAddress = _index.data(ROLE_ADDRESS).toString();
    if (transactionAddress.isEmpty()) {
      return tr("(n/a)");
    }

    QString label = AddressBookModel::instance().getAddressLabel(transactionAddress, _index.data(ROLE_PAYMENT_ID).toString(), false);
    if (!label.isEmpty()) {
      return QString("%1 (%2)").arg(label).arg(transactionAddress);
    }

    return transactionAddress;
  }

  case COLUMN_AMOUNT: {
    qint64 amount = _index.data(ROLE_AMOUNT).value<qint64>();
    QString amountStr = CurrencyAdapter::instance().formatAmount(qAbs(amount));
    return (amount < 0 ? "-" + amountStr : amountStr);
  }

  case COLUMN_PAYMENT_ID:
    return _index.data(ROLE_PAYMENT_ID);

  case COLUMN_FEE: {
    qint64 fee = _index.data(ROLE_FEE).value<qint64>();
    return CurrencyAdapter::instance().formatAmount(fee);
  }

  case COLUMN_HEIGHT:
    return QString::number(_index.data(ROLE_HEIGHT).value<quint64>());

  default:
    break;
  }

  return QVariant();
}

QVariant TransactionsModel::getDecorationRole(const QModelIndex& _index) const {
  if(_index.column() == COLUMN_STATE) {
    quint64 numberOfConfirmations = _index.data(ROLE_NUMBER_OF_CONFIRMATIONS).value<quint64>();
    TransactionState transactionState = static_cast<TransactionState>(_index.data(ROLE_STATE).value<quint8>());
    QString file;
    if (transactionState != TransactionState::ACTIVE && transactionState != TransactionState::SENDING) {
      file = QString(":icons/cancelled");
    } else if (numberOfConfirmations == 0) {
      file = QString(":icons/unconfirmed");
    } else if(numberOfConfirmations < 2) {
      file = QString(":icons/clock1");
    } else if(numberOfConfirmations < 4) {
      file = QString(":icons/clock2");
    } else if(numberOfConfirmations < 6) {
      file = QString(":icons/clock3");
    } else if(numberOfConfirmations < 8) {
      file = QString(":icons/clock4");
    } else if(numberOfConfirmations < 10) {
      file = QString(":icons/clock5");
    } else {
      file = QString(":icons/transaction");
    }
    QPixmap pixmap;
    if (!QPixmapCache::find(file, &pixmap)) {
      pixmap.load(file);
      QPixmapCache::insert(file, pixmap);
    }
    return pixmap;

  } else if (_index.column() == COLUMN_ADDRESS) {
    return _index.data(ROLE_ICON).value<QPixmap>().scaled(20, 20, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  }

  return QVariant();
}

QVariant TransactionsModel::getAlignmentRole(const QModelIndex& _index) const {
  return headerData(_index.column(), Qt::Horizontal, Qt::TextAlignmentRole);
}

QVariant TransactionsModel::getUserRole(const QModelIndex& _index, int _role, DynexCN::TransactionId _transactionId,
  DynexCN::WalletLegacyTransaction& _transaction, DynexCN::TransferId _transferId, DynexCN::WalletLegacyTransfer& _transfer) const {
  switch(_role) {

  case ROLE_STATE:
    return static_cast<quint8>(_transaction.state);
      
  case ROLE_DATE:
    return (_transaction.timestamp > 0 ? QDateTime::fromTime_t(_transaction.timestamp) : QDateTime());

  case ROLE_TYPE: {
    QString transactionAddress = _index.data(ROLE_ADDRESS).toString();
    if(_transaction.isCoinbase) {
      return static_cast<quint8>(TransactionType::MINED);
    } else if (WalletAdapter::instance().isFusionTransaction(_transaction)) {
      return static_cast<quint8>(TransactionType::FUSION);
    } else if (!transactionAddress.compare(WalletAdapter::instance().getAddress())) {
      return static_cast<quint8>(TransactionType::INOUT);
    } else if(_transaction.totalAmount < 0) {
      return static_cast<quint8>(TransactionType::OUTPUT);
    }

    return static_cast<quint8>(TransactionType::INPUT);
  }

  case ROLE_HASH:
    return QByteArray(reinterpret_cast<char*>(&_transaction.hash), sizeof(_transaction.hash)).toHex().toUpper();

  case ROLE_ADDRESS:
    return QString::fromStdString(_transfer.address);

  case ROLE_AMOUNT:
    return static_cast<qint64>(_transferId == DynexCN::WALLET_LEGACY_INVALID_TRANSFER_ID ? _transaction.totalAmount : -_transfer.amount);

  case ROLE_PAYMENT_ID:
    return NodeAdapter::instance().extractPaymentId(_transaction.extra).toLower();

  case ROLE_ICON: {
    TransactionType transactionType = static_cast<TransactionType>(_index.data(ROLE_TYPE).value<quint8>());
    return getTransactionIcon(transactionType);
  }

  case ROLE_TRANSACTION_ID:
    return QVariant::fromValue(_transactionId);

  case ROLE_HEIGHT:
    return static_cast<quint64>(_transaction.blockHeight);

  case ROLE_FEE:
    return static_cast<quint64>(_transaction.fee);

  case ROLE_NUMBER_OF_CONFIRMATIONS:
    return (_transaction.blockHeight == DynexCN::WALLET_LEGACY_UNCONFIRMED_TRANSACTION_HEIGHT ? 0 :
      NodeAdapter::instance().getLastKnownBlockHeight() - _transaction.blockHeight + 1);

  case ROLE_COLUMN:
    return headerData(_index.column(), Qt::Horizontal, ROLE_COLUMN);

  case ROLE_ROW:
    return _index.row();
  }

  return QVariant();
}

void TransactionsModel::reloadWalletTransactions() {
  beginResetModel();
  m_transfers.clear();
  m_transactionRow.clear();
  endResetModel();

  quint32 row_count = 0;
  for (DynexCN::TransactionId transactionId = 0; transactionId < WalletAdapter::instance().getTransactionCount(); ++transactionId) {
    appendTransaction(transactionId, row_count);
  }

  if (row_count > 0) {
    beginInsertRows(QModelIndex(), 0, row_count - 1);
    endInsertRows();
  }
}

void TransactionsModel::appendTransaction(DynexCN::TransactionId _transactionId, quint32& _insertedRowCount) {
  DynexCN::WalletLegacyTransaction transaction;
  if (!WalletAdapter::instance().getTransaction(_transactionId, transaction)) {
    return;
  }

  if (transaction.transferCount) {
    m_transactionRow[_transactionId] = qMakePair(m_transfers.size(), transaction.transferCount);
    for (DynexCN::TransferId transfer_id = transaction.firstTransferId;
      transfer_id < transaction.firstTransferId + transaction.transferCount; ++transfer_id) {
      m_transfers.append(TransactionTransferId(_transactionId, transfer_id));
      ++_insertedRowCount;
    }
  } else {
    m_transfers.append(TransactionTransferId(_transactionId, DynexCN::WALLET_LEGACY_INVALID_TRANSFER_ID));
    m_transactionRow[_transactionId] = qMakePair(m_transfers.size() - 1, 1);
    ++_insertedRowCount;
  }
}

void TransactionsModel::appendTransaction(DynexCN::TransactionId _transactionId) {
  if (m_transactionRow.contains(_transactionId)) {
    return;
  }

  quint32 oldRowCount = rowCount();
  quint32 insertedRowCount = 0;
  for (quint64 transactionId = m_transactionRow.size(); transactionId <= _transactionId; ++transactionId) {
    appendTransaction(transactionId, insertedRowCount);
  }

  if (insertedRowCount > 0) {
    beginInsertRows(QModelIndex(), oldRowCount, oldRowCount + insertedRowCount - 1);
    endInsertRows();
  }
}

void TransactionsModel::updateWalletTransaction(DynexCN::TransactionId _id) {
  quint32 firstRow = m_transactionRow.value(_id).first;
  quint32 lastRow = firstRow + m_transactionRow.value(_id).second - 1;
  Q_EMIT dataChanged(index(firstRow, COLUMN_DATE), index(lastRow, COLUMN_DATE));
}

void TransactionsModel::localBlockchainUpdated(quint64 _height) {
  if(rowCount() > 0) {
    Q_EMIT dataChanged(index(0, COLUMN_STATE), index(rowCount() - 1, COLUMN_STATE));
  }
}

void TransactionsModel::reset() {
  beginResetModel();
  m_transfers.clear();
  m_transactionRow.clear();
  endResetModel();
}

}
