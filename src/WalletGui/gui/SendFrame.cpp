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

#include <QMessageBox>

#include "AddressBookModel.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "NodeAdapter.h"
#include "SendFrame.h"
#include "TransferFrame.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "DynexCNConfig.h"

#include "ui_sendframe.h"

auto last_tx = std::chrono::high_resolution_clock::now();

namespace WalletGui {

SendFrame::SendFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::SendFrame) {
  m_ui->setupUi(this);
  clearAllClicked();
  m_ui->m_sendButton->setEnabled(false);
  m_ui->m_optimizeButton->setEnabled(false);

  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &SendFrame::sendTransactionCompleted,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &SendFrame::walletActualBalanceUpdated,
    Qt::QueuedConnection);

  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationProgressUpdatedSignal,
    this, &SendFrame::walletSynchronizationInProgress, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &SendFrame::walletSynchronized
    , Qt::QueuedConnection);

  m_ui->m_tickerLabel->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_feeSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  double minfee = CurrencyAdapter::instance().formatAmount(std::max(static_cast<quint64>(DynexCN::parameters::MINIMUM_FEE), NodeAdapter::instance().getMinimalFee())).toDouble();
  m_ui->m_feeSpin->setValue(CurrencyAdapter::instance().formatAmount(std::max(static_cast<quint64>(DynexCN::parameters::MAXIMUM_FEE), NodeAdapter::instance().getMinimalFee())).toDouble());
  m_ui->m_feeSpin->setMinimum(minfee);
  m_ui->m_feeSpin->setMaximum(std::max(1.0, minfee));
}

SendFrame::~SendFrame() {
}

void SendFrame::walletSynchronizationInProgress() {
  m_ui->m_sendButton->setEnabled(false);
  m_ui->m_optimizeButton->setEnabled(false);
}

void SendFrame::walletSynchronized(int _error, const QString& _error_text) {
  bool enabled = (WalletAdapter::instance().getActualBalance() > 0 && _error == 0);
  m_ui->m_sendButton->setEnabled(enabled);
  m_ui->m_optimizeButton->setEnabled(enabled);
}

void SendFrame::addRecipientClicked() {
  TransferFrame* newTransfer = new TransferFrame(m_ui->m_transfersScrollarea);
  m_ui->m_send_frame_layout->insertWidget(m_transfers.size(), newTransfer);
  m_transfers.append(newTransfer);
  if (m_transfers.size() == 1) {
    newTransfer->disableRemoveButton(true);
  } else {
    m_transfers[0]->disableRemoveButton(false);
  }

  connect(newTransfer, &TransferFrame::destroyed, [this](QObject* _obj) {
      m_transfers.removeOne(static_cast<TransferFrame*>(_obj));
      if (m_transfers.size() == 1) {
        m_transfers[0]->disableRemoveButton(true);
      }
    });

  connect(newTransfer, &TransferFrame::insertPaymentIdSignal, this, &SendFrame::insertPaymentId, Qt::QueuedConnection);
}

void SendFrame::clearAllClicked() {
  Q_FOREACH (TransferFrame* transfer, m_transfers) {
    transfer->close();
  }

  m_transfers.clear();
  addRecipientClicked();
  m_ui->m_paymentIdEdit->clear();
  //m_ui->m_feeSpin->setValue(CurrencyAdapter::instance().formatAmount(std::max(static_cast<quint64>(DynexCN::parameters::MAXIMUM_FEE), NodeAdapter::instance().getMinimalFee())).toDouble());
}

void SendFrame::optimizeClicked() {
  WalletAdapter::instance().optimize(0);
}

void SendFrame::sendClicked() {

  double minfee = CurrencyAdapter::instance().formatAmount(std::max(static_cast<quint64>(DynexCN::parameters::MINIMUM_FEE), NodeAdapter::instance().getMinimalFee())).toDouble();

  if (m_ui->m_feeSpin->value() > std::max(1.0, minfee)) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Fee is too high"), QtCriticalMsg)); 
    return;
  }
  if (m_ui->m_feeSpin->value() < minfee) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Fee is too low"), QtCriticalMsg));
    return;
  }
  
  quint64 totalAmount = 0;
  QString paymentId = m_ui->m_paymentIdEdit->text();
  if (!paymentId.isEmpty()) {
    if (!CurrencyAdapter::instance().validatePaymentId(paymentId)) {
      QCoreApplication::postEvent(
        &MainWindow::instance(),
        new ShowMessageEvent(tr("Invalid paymentID"), QtCriticalMsg));
      return;
    }
  }

  QVector<DynexCN::WalletLegacyTransfer> walletTransfers;
  Q_FOREACH (TransferFrame * transfer, m_transfers) {
    QString address = transfer->getAddress();
    if (!CurrencyAdapter::instance().validateAddress(address)) {
      QCoreApplication::postEvent(
        &MainWindow::instance(),
        new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
      return;
    }

    uint64_t amount = CurrencyAdapter::instance().parseAmount(transfer->getAmountString());
    if (amount < DynexCN::parameters::DEFAULT_DUST_THRESHOLD) {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Amount is too low"), QtCriticalMsg));
      return;
    }

    DynexCN::WalletLegacyTransfer walletTransfer;
    walletTransfer.address = address.toStdString();
    walletTransfer.amount = amount;
    walletTransfers.push_back(walletTransfer);

    totalAmount += amount;
  }

  quint64 fee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());
  if (totalAmount + fee > WalletAdapter::instance().getActualBalance()) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Insufficient balance"), QtCriticalMsg));
    return;
  }

  if (WalletAdapter::instance().isOpen()) {
    if (QMessageBox::warning(this, tr("Confirm"), tr("Send %1 DNX?").arg(CurrencyAdapter::instance().formatAmount(totalAmount)), 
      QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok) {
      WalletAdapter::instance().sendTransaction(walletTransfers, fee, paymentId, 0);

      Q_FOREACH (TransferFrame * transfer, m_transfers) {
        QString label = transfer->getLabel();
        if (!label.isEmpty()) {
          AddressBookModel::instance().addAddress(label, transfer->getAddress(), paymentId);
        }
      }
    }
  }
}

void SendFrame::sendTransactionCompleted(DynexCN::TransactionId _id, bool _error, const QString& _errorText) {
  Q_UNUSED(_id);
  if (_error) {
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(_errorText, QtCriticalMsg));
  } else {
    clearAllClicked();
  }
}

void SendFrame::walletActualBalanceUpdated(quint64 _balance) {
  m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
}

void SendFrame::insertPaymentId(QString _paymentId) {
  if (m_transfers.size() > 1 && !m_ui->m_paymentIdEdit->text().isEmpty() && m_ui->m_paymentIdEdit->text() != _paymentId) {
    if (_paymentId.isEmpty()) {
      return;
    }
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(tr("PaymentID can be set only for all transactions at once!"), QtCriticalMsg));
  }
  m_ui->m_paymentIdEdit->setText(_paymentId);
}

}
