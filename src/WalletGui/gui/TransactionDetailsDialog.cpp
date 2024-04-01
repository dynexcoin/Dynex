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

#include "CurrencyAdapter.h"
#include "TransactionDetailsDialog.h"
#include "TransactionsModel.h"

#include "ui_transactiondetailsdialog.h"

namespace WalletGui {

TransactionDetailsDialog::TransactionDetailsDialog(const QModelIndex& _index, QWidget* _parent) : QDialog(_parent),
  m_ui(new Ui::TransactionDetailsDialog), m_detailsTemplate(
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"></style></head><body>\n"
    "<b>Status: </b>%1<br>\n"
    "<b>Date: </b>%2<br>\n"
    "<b>Address: </b>%3<br>\n"
    "<b>PaymentID: </b>%4<br>\n"
    "<b>Amount: </b>%5<br>\n"
    "<b>Fee: </b>%6<br>\n"
    "<b>Transaction hash: </b>%7<br>"
    "<b>Height: </b>%8<br>"
    "<b>%9</b>%10"
    "</body></html>") {

  m_ui->setupUi(this);

  QModelIndex index = TransactionsModel::instance().index(_index.data(TransactionsModel::ROLE_ROW).toInt(),
    _index.data(TransactionsModel::ROLE_ROW).toInt());

  quint64 height = index.data(TransactionsModel::ROLE_HEIGHT).value<quint64>();
  qint64 numberOfConfirmations = index.data(TransactionsModel::ROLE_NUMBER_OF_CONFIRMATIONS).value<qint64>();
  QString amountText = index.sibling(index.row(), TransactionsModel::COLUMN_AMOUNT).data().toString() + " " +
    CurrencyAdapter::instance().getCurrencyTicker().toUpper();
  QString feeText = CurrencyAdapter::instance().formatAmount(index.data(TransactionsModel::ROLE_FEE).value<quint64>()) + " " +
    CurrencyAdapter::instance().getCurrencyTicker().toUpper();
  QString paymentID = index.sibling(index.row(), TransactionsModel::COLUMN_PAYMENT_ID).data().toString();
  QString recipients = index.data(TransactionsModel::ROLE_RECIPIENTS).toString();

  m_ui->m_detailsBrowser->setHtml(m_detailsTemplate.
    arg(numberOfConfirmations < 0 ? "-" : tr("%1 confirmations").arg(numberOfConfirmations)).
    arg(index.sibling(index.row(), TransactionsModel::COLUMN_DATE).data().toString()).
    arg(index.sibling(index.row(), TransactionsModel::COLUMN_ADDRESS).data().toString()).
    //arg(index.data(TransactionsModel::ROLE_FROM).toString()).
    //arg(index.data(TransactionsModel::ROLE_TO).toString()).
    arg(paymentID.isEmpty() ? "-" : paymentID).
    arg(amountText).
    arg(feeText).
    arg(index.sibling(index.row(), TransactionsModel::COLUMN_HASH).data().toString()).
    arg(height == DynexCN::WALLET_LEGACY_UNCONFIRMED_TRANSACTION_HEIGHT ? QString("-") : QString::number(height)).
    arg(recipients.isEmpty() ? "" : tr("Recipients:")).
    arg(recipients)
  );
}

TransactionDetailsDialog::~TransactionDetailsDialog() {
}

}
