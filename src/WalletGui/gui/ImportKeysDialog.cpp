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
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

#include "Common/StringTools.h"
#include "DynexCNCore/DynexCNTools.h"
#include "CurrencyAdapter.h"

#include "ImportKeysDialog.h"

#include "ui_importkeysdialog.h"

namespace WalletGui {

ImportKeysDialog::ImportKeysDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ImportKeysDialog) {
  m_ui->setupUi(this);
  m_ui->m_okButton->setEnabled(false);
}

ImportKeysDialog::~ImportKeysDialog() {
}

QString ImportKeysDialog::getViewKeyString() const {
  return m_ui->m_viewKeyEdit->text().trimmed();
}

QString ImportKeysDialog::getSpendKeyString() const {
  return m_ui->m_spendKeyEdit->text().trimmed();
}

QString ImportKeysDialog::getFilePath() const {
  return m_ui->m_pathEdit->text().trimmed();
}

quint32 ImportKeysDialog::getSyncHeight() const {
  return m_ui->m_syncHeight->value();
}

DynexCN::AccountKeys ImportKeysDialog::getAccountKeys() const {
  return m_keys;
}

void ImportKeysDialog::selectPathClicked() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"), nullptr, tr("Wallets (*.wallet)"));

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  m_ui->m_pathEdit->setText(filePath);
}

void ImportKeysDialog::onTextChanged(QString _text) {
  Q_UNUSED(_text);
  if (getViewKeyString().isEmpty() || getSpendKeyString().isEmpty() || getViewKeyString().size() != 2*sizeof(Crypto::SecretKey) 
  	|| getSpendKeyString().size() != 2*sizeof(Crypto::SecretKey)) {
    m_ui->m_okButton->setEnabled(false);
  } else {
    m_ui->m_okButton->setEnabled(true);
  }
}

void ImportKeysDialog::onAccept() {
  QString viewKeyString = getViewKeyString().trimmed();
  QString spendKeyString = getSpendKeyString().trimmed();

  if (viewKeyString.isEmpty() || spendKeyString.isEmpty()) {
    return;
  }

  std::string data;

  std::string private_spend_key_string = spendKeyString.toStdString();
  std::string private_view_key_string = viewKeyString.toStdString();

  Crypto::SecretKey private_spend_key_hash;
  Crypto::SecretKey private_view_key_hash;

  size_t size;
  if (!Common::fromHex(private_spend_key_string, &private_spend_key_hash, sizeof(private_spend_key_hash), size) || size != sizeof(private_spend_key_hash)) {
    QMessageBox::warning(this, tr("Key is not valid"), tr("The private spend key you entered is not valid."), QMessageBox::Ok);
    return;
  }
  if (!Common::fromHex(private_view_key_string, &private_view_key_hash, sizeof(private_view_key_hash), size) || size != sizeof(private_view_key_hash)) {
    QMessageBox::warning(this, tr("Key is not valid"), tr("The private view key you entered is not valid."), QMessageBox::Ok);
    return;
  }

  m_keys.spendSecretKey = private_spend_key_hash;
  m_keys.viewSecretKey = private_view_key_hash;

  Crypto::secret_key_to_public_key(m_keys.spendSecretKey, m_keys.address.spendPublicKey);
  Crypto::secret_key_to_public_key(m_keys.viewSecretKey, m_keys.address.viewPublicKey);

  if (getFilePath().isEmpty()) {
    QMessageBox::critical(nullptr, tr("File path is empty"), tr("Please enter the path where to save the wallet file and its name."), QMessageBox::Ok);
    return;
  }

  accept();
}

}
