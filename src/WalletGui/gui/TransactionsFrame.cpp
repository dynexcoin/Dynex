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

#include <QFileDialog>
#include <QLabel>
#include <QClipboard>

#include "MainWindow.h"
#include "SortedTransactionsModel.h"
#include "TransactionsFrame.h"
#include "TransactionDetailsDialog.h"
#include "TransactionsListModel.h"
#include "TransactionsModel.h"

#include "ui_transactionsframe.h"

namespace WalletGui {

TransactionsFrame::TransactionsFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::TransactionsFrame),
  m_transactionsModel(new TransactionsListModel) {
  m_ui->setupUi(this);
  m_ui->m_transactionsView->setModel(m_transactionsModel.data());
  m_ui->m_transactionsView->verticalHeader()->setDefaultSectionSize(20);
  m_ui->m_transactionsView->horizontalHeader()->setMinimumSectionSize(20);
  m_ui->m_transactionsView->horizontalHeader()->resizeSection(TransactionsModel::COLUMN_STATE, 20);
  m_ui->m_transactionsView->horizontalHeader()->setSectionResizeMode(TransactionsModel::COLUMN_STATE, QHeaderView::Fixed);
  m_ui->m_transactionsView->horizontalHeader()->resizeSection(TransactionsModel::COLUMN_DATE, 110);
  m_ui->m_transactionsView->horizontalHeader()->resizeSection(TransactionsModel::COLUMN_AMOUNT, 105);
  m_ui->m_transactionsView->horizontalHeader()->resizeSection(TransactionsModel::COLUMN_ADDRESS, 385);
  m_ui->m_transactionsView->horizontalHeader()->resizeSection(TransactionsModel::COLUMN_PAYMENT_ID, 90);
  m_ui->m_transactionsView->horizontalHeader()->setStretchLastSection(true);
  m_ui->m_transactionsView->setWordWrap(false);
  m_ui->m_transactionsView->setTextElideMode(Qt::ElideMiddle);
  m_ui->m_transactionsView->setSelectionMode(QAbstractItemView::SingleSelection);
  m_ui->m_transactionsView->setSelectionBehavior(QAbstractItemView::SelectRows);

  m_ui->m_transactionsView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->m_transactionsView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));

  m_contextMenu = new QMenu();
  m_contextMenu->addAction(QString(tr("Copy date")), this, SLOT(copyDate()));
  m_contextMenu->addAction(QString(tr("Copy amount")), this, SLOT(copyAmount()));
  m_contextMenu->addAction(QString(tr("Copy address")), this, SLOT(copyAddress()));
  m_contextMenu->addAction(QString(tr("Copy paymentID")), this, SLOT(copyPaymentID()));
  m_contextMenu->addAction(QString(tr("Copy hash")), this, SLOT(copyHash()));
  m_contextMenu->addSeparator();
  m_contextMenu->addAction(QString(tr("Show details")), this, SLOT(showDetails()));
}

TransactionsFrame::~TransactionsFrame() {
  delete m_contextMenu;
}

void TransactionsFrame::scrollToTransaction(const QModelIndex& _index) {
  QModelIndex sortedModelIndex = SortedTransactionsModel::instance().mapFromSource(_index);
  QModelIndex index = static_cast<QSortFilterProxyModel*>(m_ui->m_transactionsView->model())->mapFromSource(sortedModelIndex);
  m_ui->m_transactionsView->scrollTo(index);
  m_ui->m_transactionsView->setFocus();
  m_ui->m_transactionsView->setCurrentIndex(index);
}

void TransactionsFrame::exportToCsv() {
  QString file = QFileDialog::getSaveFileName(&MainWindow::instance(), tr("Select CSV file"), nullptr, "CSV (*.csv)");
  if (!file.isEmpty()) {
    QByteArray csv = TransactionsModel::instance().toCsv();
    QFile f(file);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      f.write(csv);
      f.close();
    }
  }
}

void TransactionsFrame::showTransactionDetails(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  TransactionDetailsDialog dlg(_index, &MainWindow::instance());
  dlg.exec();
}

void TransactionsFrame::showDetails() {
	showTransactionDetails(index);
}

void TransactionsFrame::copyAddress() {
  QApplication::clipboard()->setText(index.sibling(index.row(), TransactionsModel::COLUMN_ADDRESS).data().toString());
}

void TransactionsFrame::copyHash() {
  QApplication::clipboard()->setText(index.sibling(index.row(), TransactionsModel::COLUMN_HASH).data().toString());
}

void TransactionsFrame::copyAmount() {
  QApplication::clipboard()->setText(index.sibling(index.row(), TransactionsModel::COLUMN_AMOUNT).data().toString());
}

void TransactionsFrame::copyPaymentID() {
  QApplication::clipboard()->setText(index.sibling(index.row(), TransactionsModel::COLUMN_PAYMENT_ID).data().toString());
}

void TransactionsFrame::copyDate() {
  QApplication::clipboard()->setText(index.sibling(index.row(), TransactionsModel::COLUMN_DATE).data().toString());
}

void TransactionsFrame::onCustomContextMenu(const QPoint& point) {
  index = m_ui->m_transactionsView->indexAt(point);
  if (!index.isValid()) {
    return;
  }
  m_contextMenu->exec(m_ui->m_transactionsView->mapToGlobal(point));
}

}
