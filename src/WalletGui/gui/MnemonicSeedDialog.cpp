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

#include "MnemonicSeedDialog.h"
#include "ui_mnemonicseeddialog.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"
#include "Mnemonics/electrum-words.h"
#include "Settings.h"

namespace WalletGui {

MnemonicSeedDialog::MnemonicSeedDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::MnemonicSeedDialog) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &MnemonicSeedDialog::walletOpened, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &MnemonicSeedDialog::walletClosed, Qt::QueuedConnection);
  initLanguages();
  m_ui->m_languageCombo->setCurrentIndex(m_ui->m_languageCombo->findData("English", Qt::DisplayRole));
}

MnemonicSeedDialog::~MnemonicSeedDialog() {
}

void MnemonicSeedDialog::walletOpened() {
  QString mnemonicSeed = WalletAdapter::instance().getMnemonicSeed(m_ui->m_languageCombo->currentText());
  m_ui->m_mnemonicSeedEdit->setText(mnemonicSeed);
}

void MnemonicSeedDialog::walletClosed() {
  m_ui->m_mnemonicSeedEdit->clear();
}

void MnemonicSeedDialog::initLanguages() {
  std::vector<std::string> languages;
  Crypto::ElectrumWords::get_language_list(languages);
  for (size_t i = 0; i < languages.size(); ++i)
  {
    m_ui->m_languageCombo->addItem(QString::fromStdString(languages[i]));
  }
}

void MnemonicSeedDialog::languageChanged() {
  QString mnemonicSeed = WalletAdapter::instance().getMnemonicSeed(m_ui->m_languageCombo->currentText());
  m_ui->m_mnemonicSeedEdit->setText(mnemonicSeed);
}

}
