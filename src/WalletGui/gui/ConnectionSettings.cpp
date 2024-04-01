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

#include <iostream>
#include <QApplication>
#include <QMessageBox>
#include <QUrl>
#include <QRegularExpression>

#include "ui_connectionsettingsdialog.h"
#include "ConnectionSettings.h"
#include "CurrencyAdapter.h"
#include "Settings.h"


namespace WalletGui {

ConnectionSettingsDialog::ConnectionSettingsDialog(QWidget *_parent) : QDialog(_parent), m_ui(new Ui::ConnectionSettingsDialog) {
  m_ui->setupUi(this);

  QString connection = Settings::instance().getConnection();
  if (connection.compare("auto") == 0) {
    m_ui->radioButton_1->setChecked(true);
  } else if (connection.compare("embedded") == 0) {
    m_ui->radioButton_2->setChecked(true);
  } else if (connection.compare("local") == 0) {
    m_ui->radioButton_3->setChecked(true);
  } else if (connection.compare("remote") == 0) {
    m_ui->radioButton_4->setChecked(true);
  }

  m_ui->m_localDaemonPort->setValue(Settings::instance().getLocalDaemonPort());
  m_ui->m_remoteNode->setText(Settings::instance().getRemoteNode());
  m_ui->m_bindIp->setText(Settings::instance().getP2pBindIp());
  m_ui->m_bindPort->setValue(Settings::instance().getP2pBindPort());
  m_ui->m_externalPort->setValue(Settings::instance().getP2pExternalPort());

  QRegularExpressionValidator* ipValidator = new QRegularExpressionValidator(QRegularExpression("^(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])(\\.(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])){3}$"), this);
  m_ui->m_bindIp->setValidator(ipValidator);

  QRegularExpressionValidator* hostValidator = new QRegularExpressionValidator(QRegularExpression("^((?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])(\\.(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])){3}|[a-zA-Z0-9-\\.]+)(:[0-9]{1,5})?$"), this);
  m_ui->m_remoteNode->setValidator(hostValidator);

  // disable options set from cmdline
  m_ui->m_bindIp->setEnabled(!Settings::instance().hasP2pBindIp());
  m_ui->m_bindPort->setEnabled(!Settings::instance().hasP2pBindPort());
  m_ui->m_externalPort->setEnabled(!Settings::instance().hasP2pExternalPort());
}

ConnectionSettingsDialog::~ConnectionSettingsDialog() {
}

void ConnectionSettingsDialog::onAccept() {
  QString connectionMode;
  if (m_ui->radioButton_1->isChecked()) {
    connectionMode = "auto";
  } else if (m_ui->radioButton_2->isChecked()) {
    connectionMode = "embedded";
  } else if (m_ui->radioButton_3->isChecked()) {
    connectionMode = "local";
  } else if(m_ui->radioButton_4->isChecked()) {
    connectionMode = "remote";
  }

  QString node = m_ui->m_remoteNode->text().trimmed();
  if (!node.isEmpty() || connectionMode == "remote") {
    QUrl url = QUrl::fromUserInput(node);
    if (!url.isValid() || !url.path().isEmpty() || url.port() > 65535) {
      QMessageBox::warning(this, tr("Error"), tr("Invalid remote node host"), QMessageBox::Ok);
      return;
    }
  }

  QString ip = m_ui->m_bindIp->text().trimmed();
  if (ip.isEmpty()) ip = "0.0.0.0";
  if (!QRegularExpression("^(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])(\\.(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])){3}$").match(ip).hasMatch()) {
    QMessageBox::warning(this, tr("Error"), tr("Invalid bind ip"), QMessageBox::Ok);
    return;
  }

  Settings::instance().setConnection(connectionMode);
  Settings::instance().setRemoteNode(m_ui->m_remoteNode->text().trimmed());
  Settings::instance().setLocalDaemonPort(m_ui->m_localDaemonPort->value());
  Settings::instance().setP2pBindIp(m_ui->m_bindIp->text().trimmed());
  Settings::instance().setP2pBindPort(m_ui->m_bindPort->value());
  Settings::instance().setP2pExternalPort(m_ui->m_externalPort->value());

  QMessageBox::information(this, tr("Connection settings changed"), tr("Connection mode will be changed after restarting the wallet."), QMessageBox::Ok);
  accept();
}

}