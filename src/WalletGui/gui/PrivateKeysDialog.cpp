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

#include "PrivateKeysDialog.h"
#include "ui_privatekeysdialog.h"
#include <QClipboard>
#include <Common/Base58.h>
#include <Common/StringTools.h>
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"

namespace WalletGui {

PrivateKeysDialog::PrivateKeysDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::PrivateKeysDialog) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &PrivateKeysDialog::walletOpened, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &PrivateKeysDialog::walletClosed, Qt::QueuedConnection);
}

PrivateKeysDialog::~PrivateKeysDialog() {
}

void PrivateKeysDialog::walletOpened() {
  DynexCN::AccountKeys keys;
  WalletAdapter::instance().getAccountKeys(keys);

  QString privateKeys = QString::fromStdString(Tools::Base58::encode_addr(CurrencyAdapter::instance().getAddressPrefix(),
    std::string(reinterpret_cast<char*>(&keys), sizeof(keys))));

  m_ui->m_privateKeyEdit->setText(privateKeys);

  QString spendSecretKey = QString::fromStdString(Common::podToHex(keys.spendSecretKey));
  QString viewSecretKey = QString::fromStdString(Common::podToHex(keys.viewSecretKey));

  m_ui->m_spendSecretKeyEdit->setText(spendSecretKey);
  m_ui->m_viewSecretKeyEdit->setText(viewSecretKey);
}

void PrivateKeysDialog::walletClosed() {
  m_ui->m_privateKeyEdit->clear();
  m_ui->m_spendSecretKeyEdit->clear();
  m_ui->m_viewSecretKeyEdit->clear();
}

void PrivateKeysDialog::copyKey() {
  QApplication::clipboard()->setText(m_ui->m_privateKeyEdit->toPlainText());
}

}
