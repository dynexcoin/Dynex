// Copyright (c) 2022-2024, Dynex Developers
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

#include <QClipboard>
#include <QFileDialog>
#include <QBuffer>
#include <QTextStream>

#include "CurrencyAdapter.h"
#include "WalletAdapter.h"

#include "BalanceProofDialog.h"
#include "ui_balanceproofdialog.h"

namespace WalletGui {

BalanceProofDialog::BalanceProofDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::BalanceProofDialog) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &BalanceProofDialog::updateProof, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal, this, &BalanceProofDialog::updateProof, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &BalanceProofDialog::walletOpened, Qt::QueuedConnection);
}

BalanceProofDialog::~BalanceProofDialog() {
}

void BalanceProofDialog::walletOpened() {
  QString address = WalletAdapter::instance().getAddress();
  m_ui->m_addressLineEdit->setText(address);
  m_amount = 0;
  updateProof();
}

void BalanceProofDialog::updateProof() {
  quint64 balance = WalletAdapter::instance().getActualBalance();
  quint64 pending = WalletAdapter::instance().getPendingBalance();
  QString currency = CurrencyAdapter::instance().getCurrencyTicker().toUpper();

  if (pending) {
    m_ui->m_balance->setText(tr("Balance: %1 %2   Pending: %3 %2").arg(CurrencyAdapter::instance().formatAmount(balance)).arg(currency).arg(CurrencyAdapter::instance().formatAmount(pending)));
  } else {
    m_ui->m_balance->setText(tr("Balance: %1 %2").arg(CurrencyAdapter::instance().formatAmount(balance)).arg(currency));
  }

  if (balance == 0) {
    m_ui->m_signatureEdit->setText("");
    m_amount = 0;
  } else if (balance != m_amount) {
    m_ui->m_signatureEdit->setText(WalletAdapter::instance().getReserveProof(balance));
    m_amount = balance;
  }
  m_ui->m_copyProofButton->setDisabled(balance == 0);
  m_ui->m_saveProofButton->setDisabled(balance == 0);
}

void BalanceProofDialog::copyProof() {
  QApplication::clipboard()->setText(m_ui->m_signatureEdit->toPlainText());
}

void BalanceProofDialog::saveProof() {
  QString file = QFileDialog::getSaveFileName(this, tr("Save as"), QDir::homePath(), "TXT (*.txt)");
  if (file.isEmpty()) return;
  QFile f(file);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
  QTextStream outputStream(&f);
  outputStream << m_ui->m_signatureEdit->toPlainText();
  f.close();
}

}
