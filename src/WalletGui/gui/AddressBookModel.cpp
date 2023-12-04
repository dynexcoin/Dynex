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

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "WalletAdapter.h"
#include "AddressBookModel.h"
#include "Settings.h"

namespace WalletGui {

AddressBookModel& AddressBookModel::instance() {
  static AddressBookModel inst;
  return inst;
}

AddressBookModel::AddressBookModel() : QAbstractItemModel() {
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &AddressBookModel::walletInitCompleted, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &AddressBookModel::reset, Qt::QueuedConnection);
}

AddressBookModel::~AddressBookModel() {
}

int AddressBookModel::columnCount(const QModelIndex& _parent) const {
  return TOTAL_COLUMNS;
}

QVariant AddressBookModel::data(const QModelIndex& _index, int _role) const {
  if (!_index.isValid()) {
    return QVariant();
  }

  QJsonObject address = m_addressBook.at(_index.row()).toObject();

  switch (_role) {
  case Qt::DisplayRole:
    switch (_index.column()) {
    case COLUMN_LABEL:
      return _index.data(ROLE_LABEL);
    case COLUMN_ADDRESS:
      return _index.data(ROLE_ADDRESS);
    case COLUMN_PAYMENTID:
      return _index.data(ROLE_PAYMENTID);
    default:
      return QVariant();
    }

  case ROLE_LABEL:
    return address.value("label");
  case ROLE_ADDRESS:
    return address.value("address");
  case ROLE_PAYMENTID:
    return address.value("paymentid");
  default:
    return QVariant();
  }

  return QVariant();
}

Qt::ItemFlags AddressBookModel::flags(const QModelIndex& _index) const {
  return (Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable);
}

QVariant AddressBookModel::headerData(int _section, Qt::Orientation _orientation, int _role) const {
  if (_orientation != Qt::Horizontal || _role != Qt::DisplayRole) {
    return QVariant();
  }

  switch (_section) {
  case COLUMN_LABEL:
    return tr("Label");
  case COLUMN_ADDRESS:
    return tr("Address");
  case COLUMN_PAYMENTID:
    return tr("PaymentID");
  }

  return QVariant();
}

QModelIndex AddressBookModel::index(int _row, int _column, const QModelIndex& _parent) const {
  if (_parent.isValid()) {
    return QModelIndex();
  }

  return createIndex(_row, _column, _row);
}

QModelIndex AddressBookModel::parent(const QModelIndex& _index) const {
  return QModelIndex();
}

int AddressBookModel::rowCount(const QModelIndex& _parent) const {
  return m_addressBook.size();
}

bool AddressBookModel::addAddress(const QString& _label, const QString& _address, const QString& _paymentId, const QString& _old_label) {
  if (_label.isEmpty() || _address.isEmpty()) return false;

  QString addr_label = getAddressLabel(_address, _paymentId, true);

  if (!addr_label.isEmpty() && addr_label != _old_label) return false; // duplicate address+id
  if (_label == _old_label && _old_label == addr_label) return true; // nothing to do
  if (getAddressIndex(_label).isValid() && _label != _old_label) return false; // duplicate label

  QJsonObject newAddress;
  newAddress.insert("label", _label);
  newAddress.insert("address", _address);
  newAddress.insert("paymentid", _paymentId);

  if (_old_label.isEmpty()) {
    // add new
    beginInsertRows(QModelIndex(), m_addressBook.size(), m_addressBook.size());
    m_addressBook.append(newAddress);
    endInsertRows();
    saveAddressBook();
    return true;
  }

  for(int i = 0; i < m_addressBook.size(); i++) {
    if (m_addressBook.at(i).toObject().value("label") == _old_label) {
      m_addressBook.replace(i, newAddress);
      saveAddressBook();
      return true;
    }
  }
  
  return false;
}

void AddressBookModel::removeAddress(const QString& _label) {
  QModelIndex index = getAddressIndex(_label);
  if (!index.isValid()) return;
  int row = index.row();

  beginRemoveRows(QModelIndex(), row, row);
  for(int i = 0; i < m_addressBook.size(); i++) {
    if (m_addressBook.at(i).toObject().value("label") == _label) {
      m_addressBook.removeAt(i);
      break;
    }
  }
  endRemoveRows();
  saveAddressBook();
}

void AddressBookModel::reset() {
  beginResetModel();
  while (!m_addressBook.empty()) {
    m_addressBook.removeFirst();
  }

  endResetModel();
}

void AddressBookModel::saveAddressBook() {
  QFile addressBookFile(Settings::instance().getAddressBookFile());
  if (addressBookFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QByteArray file_content = QJsonDocument(m_addressBook).toJson(QJsonDocument::Compact);
    addressBookFile.write(file_content);
    addressBookFile.close();
  }
}

const QString AddressBookModel::getAddressLabel(const QString& _address, const QString& _paymentId, bool _strict_match) {
  if (!_paymentId.isEmpty()) {
    QModelIndexList matches = match(AddressBookModel::index(0, COLUMN_PAYMENTID, QModelIndex()), Qt::DisplayRole, _paymentId, -1, 
      Qt::MatchFlags(Qt::MatchExactly|Qt::MatchRecursive));

    for(int i = 0; i < matches.size(); i++) {
      QModelIndex index = matches.at(i);
      if (index.data(ROLE_ADDRESS).toString() == _address) {
        return index.data(ROLE_LABEL).toString();
      }
    }
    if (_strict_match) return QString();
  }

  QModelIndexList matches = match(AddressBookModel::index(0, COLUMN_ADDRESS, QModelIndex()), Qt::DisplayRole, _address, -1, 
    Qt::MatchFlags(Qt::MatchExactly|Qt::MatchRecursive));

  if (matches.size() == 0) return QString();

  for(int i = 0; i < matches.size(); i++) {
    QModelIndex index = matches.at(i);
    if (index.data(ROLE_PAYMENTID).toString().isEmpty()) {
      return index.data(ROLE_LABEL).toString();
    }
  }

  if (!_strict_match) return matches.at(0).data(ROLE_LABEL).toString();

  return QString();
}

QModelIndex AddressBookModel::getAddressIndex(const QString& _label) {
  if (_label.isEmpty()) return QModelIndex();
  return match(AddressBookModel::index(0, COLUMN_LABEL, QModelIndex()), Qt::DisplayRole, _label, 1, 
    Qt::MatchFlags(Qt::MatchExactly|Qt::MatchRecursive)).value(0);
}


void AddressBookModel::walletInitCompleted(int _error, const QString& _error_text) {
  if (!_error) {
    QFile addressBookFile(Settings::instance().getAddressBookFile());
    if (addressBookFile.open(QIODevice::ReadOnly)) {
      QByteArray file_content = addressBookFile.readAll();
      QJsonDocument doc = QJsonDocument::fromJson(file_content);
      if (!doc.isNull()) {
        m_addressBook = doc.array();
      }

      addressBookFile.close();
      if (!m_addressBook.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, m_addressBook.size() - 1);
        endInsertRows();
      }
    }
  }
}

}
