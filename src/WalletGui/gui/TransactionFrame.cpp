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

#include <QFontDatabase>

#include "MainWindow.h"
#include "TransactionFrame.h"
#include "TransactionsModel.h"

#include "ui_transactionframe.h"

namespace WalletGui {

class RecentTransactionDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  RecentTransactionDelegate(QObject* _parent) : QStyledItemDelegate(_parent) {

  }

  ~RecentTransactionDelegate() {
  }

  void setEditorData(QWidget* _editor, const QModelIndex& _index) const Q_DECL_OVERRIDE {
    switch(_index.column()) {
    case TransactionsModel::COLUMN_AMOUNT:
    case TransactionsModel::COLUMN_HASH:
    case TransactionsModel::COLUMN_DATE:
      static_cast<QLabel*>(_editor)->setText(_index.data().toString());
      return;
    case TransactionsModel::COLUMN_TYPE:
      static_cast<QLabel*>(_editor)->setPixmap(_index.data(TransactionsModel::ROLE_ICON).value<QPixmap>());
      return;
    default:
      return;
    }
  }
};

TransactionFrame::TransactionFrame(const QModelIndex& _index, QWidget* _parent) : QFrame(_parent),
  m_ui(new Ui::TransactionFrame), m_dataMapper(this), m_index(_index) {
  m_ui->setupUi(this);
  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPixelSize(11);
  m_ui->m_hashLabel->setFont(font);

  m_dataMapper.setModel(const_cast<QAbstractItemModel*>(m_index.model()));
  m_dataMapper.setItemDelegate(new RecentTransactionDelegate(this));
  m_dataMapper.addMapping(m_ui->m_iconLabel, TransactionsModel::COLUMN_TYPE);
  m_dataMapper.addMapping(m_ui->m_amountLabel, TransactionsModel::COLUMN_AMOUNT);
  m_dataMapper.addMapping(m_ui->m_timeLabel, TransactionsModel::COLUMN_DATE);
  m_dataMapper.addMapping(m_ui->m_hashLabel, TransactionsModel::COLUMN_HASH);
  m_dataMapper.setCurrentModelIndex(m_index);
}

TransactionFrame::~TransactionFrame() {
}

void TransactionFrame::mousePressEvent(QMouseEvent* _event) {
  MainWindow::instance().scrollToTransaction(TransactionsModel::instance().index(m_index.data(TransactionsModel::ROLE_ROW).toInt(), 0));
}

}

#include "TransactionFrame.moc"
