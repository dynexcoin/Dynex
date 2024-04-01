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

#include <QApplication>
#include <QClipboard>

#include "CurrencyAdapter.h"
#include "AddressBookModel.h"
#include "AddressBookFrame.h"
#include "MainWindow.h"
#include "NewAddressDialog.h"
#include "WalletEvents.h"

#include "ui_addressbookframe.h"

namespace WalletGui {

AddressBookFrame::AddressBookFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::AddressBookFrame) {
  m_ui->setupUi(this);
  //m_ui->m_addressBookView->setModel(&AddressBookModel::instance());
  m_proxyModel = new QSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(&AddressBookModel::instance());
  m_ui->m_addressBookView->setModel(m_proxyModel);

  //m_ui->m_addressBookView->header()->setStretchLastSection(false);
  m_ui->m_addressBookView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
  m_ui->m_addressBookView->setSortingEnabled(true);
  m_ui->m_addressBookView->sortByColumn(0, Qt::AscendingOrder);
  m_ui->m_addressBookView->setRootIsDecorated(false);
  m_ui->m_addressBookView->setTextElideMode(Qt::ElideMiddle);

  connect(m_ui->m_addressBookView->selectionModel(), &QItemSelectionModel::currentChanged, this, &AddressBookFrame::currentAddressChanged);

  m_ui->m_addressBookView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ui->m_addressBookView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));

  m_contextMenu = new QMenu();
  m_contextMenu->addAction(QString(tr("Copy address")), this, SLOT(copyClicked()));
  m_contextMenu->addAction(QString(tr("Copy paymentID")), this, SLOT(copyPaymentIdClicked()));
  m_contextMenu->addAction(QString(tr("Edit")), this, SLOT(editClicked()));
  m_contextMenu->addAction(QString(tr("Delete")), this, SLOT(deleteClicked()));

  //m_ui->m_addressBookView->resizeColumnToContents(0);
  m_ui->m_addressBookView->setColumnWidth(0, 75);
  m_ui->m_addressBookView->setColumnWidth(1, 700);
  //m_ui->m_addressBookView->setColumnWidth(2, 300);
}

AddressBookFrame::~AddressBookFrame() {
  delete m_contextMenu;
  delete m_proxyModel;
}

void AddressBookFrame::onCustomContextMenu(const QPoint &point) {
  QModelIndex index = m_ui->m_addressBookView->indexAt(point);
  if (!index.isValid()) {
    return;
  }
  m_contextMenu->exec(m_ui->m_addressBookView->mapToGlobal(point));
}

void AddressBookFrame::addClicked() {
  NewAddressDialog dlg(&MainWindow::instance());
  dlg.exec();
}

void AddressBookFrame::copyClicked() {
  QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
}

void AddressBookFrame::copyPaymentIdClicked() {
  QApplication::clipboard()->setText(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_PAYMENTID).toString());
}

void AddressBookFrame::deleteClicked() {
  AddressBookModel::instance().removeAddress(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_LABEL).toString());
  currentAddressChanged(m_ui->m_addressBookView->currentIndex());
}

void AddressBookFrame::editClicked() {
  NewAddressDialog dlg(&MainWindow::instance());
  dlg.setWindowTitle(QString(tr("Edit address")));
  dlg.setLabel(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_LABEL).toString());
  dlg.setAddress(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_ADDRESS).toString());
  dlg.setPaymentId(m_ui->m_addressBookView->currentIndex().data(AddressBookModel::ROLE_PAYMENTID).toString());
  dlg.exec();
}

void AddressBookFrame::currentAddressChanged(const QModelIndex& _index) {
  m_ui->m_copyAddressButton->setEnabled(_index.isValid());
  m_ui->m_deleteAddressButton->setEnabled(_index.isValid());
  m_ui->m_copyPaymentIdButton->setEnabled(!_index.data(AddressBookModel::ROLE_PAYMENTID).toString().isEmpty());
}

void AddressBookFrame::addressDoubleClicked(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }
  editClicked();
}

}
