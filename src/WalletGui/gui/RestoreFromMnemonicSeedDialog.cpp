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
#include "RestoreFromMnemonicSeedDialog.h"
#include "Mnemonics/electrum-words.h"

extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}

#include "ui_restorefrommnemonicseeddialog.h"

namespace WalletGui {

RestoreFromMnemonicSeedDialog::RestoreFromMnemonicSeedDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::RestoreFromMnemonicSeedDialog) {
  m_ui->setupUi(this);
  m_ui->m_okButton->setEnabled(false);
}

RestoreFromMnemonicSeedDialog::~RestoreFromMnemonicSeedDialog() {
}

QString RestoreFromMnemonicSeedDialog::getSeedString() const {
  return m_ui->m_seedEdit->toPlainText().trimmed();
}

QString RestoreFromMnemonicSeedDialog::getFilePath() const {
  return m_ui->m_pathEdit->text().trimmed();
}

quint32 RestoreFromMnemonicSeedDialog::getSyncHeight() const {
  return m_ui->m_syncHeight->value();
}

DynexCN::AccountKeys RestoreFromMnemonicSeedDialog::getAccountKeys() const {
  return m_keys;
}

void RestoreFromMnemonicSeedDialog::selectPathClicked() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Wallet file"), nullptr, tr("Wallets (*.wallet)"));

  if (!filePath.isEmpty() && !filePath.endsWith(".wallet")) {
    filePath.append(".wallet");
  }

  m_ui->m_pathEdit->setText(filePath);
}

void RestoreFromMnemonicSeedDialog::onTextChanged() {
  wordCount = m_ui->m_seedEdit->toPlainText().split(QRegularExpression("(\\s|\\n|\\r)+"), Qt::SkipEmptyParts).count();
  if(wordCount == 25) {
    m_ui->m_okButton->setEnabled(true);
    m_ui->m_errorLabel->setText("OK");
    m_ui->m_errorLabel->setStyleSheet("QLabel { color : green; }");
  } else {
    m_ui->m_okButton->setEnabled(false);
    m_ui->m_errorLabel->setText(QString::number(wordCount));
    m_ui->m_errorLabel->setStyleSheet("QLabel { color : red; }");
  }
}

void RestoreFromMnemonicSeedDialog::onAccept() {
  std::string seed_language = "";
  QString seedString = getSeedString();
  if (!Crypto::ElectrumWords::words_to_bytes(seedString.toStdString(), m_keys.spendSecretKey, seed_language)) {
    QMessageBox::critical(nullptr, tr("Mnemonic seed is not correct"), tr("There must be an error in mnemonic seed. Make sure you entered it correctly."), QMessageBox::Ok);
    return;
  } else {
    Crypto::secret_key_to_public_key(m_keys.spendSecretKey,m_keys.address.spendPublicKey);
    Crypto::SecretKey second;
    keccak((uint8_t *)&m_keys.spendSecretKey, sizeof(Crypto::SecretKey), (uint8_t *)&second, sizeof(Crypto::SecretKey));
    Crypto::generate_deterministic_keys(m_keys.address.viewPublicKey,m_keys.viewSecretKey,second);
  }

  if (getFilePath().isEmpty()) {
    QMessageBox::critical(nullptr, tr("File path is empty"), tr("Please enter the path where to save the wallet file and its name."), QMessageBox::Ok);
    return;
  }

  accept();
}

}
