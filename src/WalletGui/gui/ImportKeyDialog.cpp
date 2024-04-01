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

#include "Common/Base58.h"
#include "Common/StringTools.h"
#include "DynexCNCore/DynexCNTools.h"
#include "CurrencyAdapter.h"

#include "ImportKeyDialog.h"

#include "ui_importkeydialog.h"

namespace WalletGui {

ImportKeyDialog::ImportKeyDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::ImportKeyDialog) {
  m_ui->setupUi(this);
  m_ui->m_okButton->setEnabled(false);
}

ImportKeyDialog::~ImportKeyDialog() {
}

QString ImportKeyDialog::getKeyString() const {
  return m_ui->m_keyEdit->toPlainText().trimmed();
}

QString ImportKeyDialog::getFilePath() const {
  return m_ui->m_pathEdit->text().trimmed();
}

quint32 ImportKeyDialog::getSyncHeight() const {
  return m_ui->m_syncHeight->value();
}

DynexCN::AccountKeys ImportKeyDialog::getAccountKeys() const {
  return m_keys;
}

void ImportKeyDialog::selectPathClicked() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"), nullptr, tr("Wallets (*.wallet)"));

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  m_ui->m_pathEdit->setText(filePath);
}

void ImportKeyDialog::onTextChanged() {
  if (getKeyString().isEmpty() || getKeyString().size() != 185) {
    m_ui->m_okButton->setEnabled(false);
  } else {
    m_ui->m_okButton->setEnabled(true);
  }
}

void ImportKeyDialog::onAccept() {
  uint64_t addressPrefix;
  std::string data;
  QString keyString = getKeyString().trimmed();
  if (!keyString.isEmpty() && Tools::Base58::decode_addr(keyString.toStdString(), addressPrefix, data) 
  	&& addressPrefix == CurrencyAdapter::instance().getAddressPrefix() && data.size() == sizeof(m_keys)) {
    if (!DynexCN::fromBinaryArray(m_keys, Common::asBinaryArray(data))) {
      QMessageBox::warning(nullptr, tr("Wallet keys are not valid"), tr("Failed to parse account keys"), QMessageBox::Ok);
      return;
    }
  } else {
    QMessageBox::warning(nullptr, tr("Wallet keys are not valid"), tr("The private keys you entered are not valid."), QMessageBox::Ok);
    return;
  }

  QString filePath = getFilePath();
  if (filePath.isEmpty()) {
    QMessageBox::critical(nullptr, tr("File path is empty"), tr("Please enter the path where to save the wallet file and its name."), QMessageBox::Ok);
    return;
  }

  accept();
}

}
