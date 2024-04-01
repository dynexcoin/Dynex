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
#include <QFile>
#include <QJsonDocument>
#include <QSettings>
#include <QStandardPaths>

#include <Common/Util.h>

#include "CommandLineParser.h"
#include "CurrencyAdapter.h"
#include "Settings.h"

#include "version.h"

namespace WalletGui {

Settings& Settings::instance() {
  static Settings inst;
  return inst;
}

Settings::Settings() : QObject(), m_cmdLineParser(nullptr) {
}

Settings::~Settings() {
}

void Settings::setCommandLineParser(CommandLineParser* _cmdLineParser) {
  Q_CHECK_PTR(_cmdLineParser);
  m_cmdLineParser = _cmdLineParser;
}

void Settings::load() {
  QFile cfgFile(getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".cfg"));
  if (cfgFile.open(QIODevice::ReadOnly)) {
    m_settings = QJsonDocument::fromJson(cfgFile.readAll()).object();
    cfgFile.close();
  } 
}

bool Settings::isTestnet() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->hasTestnetOption();
}

bool Settings::hasAllowLocalIpOption() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->hasAllowLocalIpOption();
}

bool Settings::hasHideMyPortOption() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return true; //m_cmdLineParser->hasHideMyPortOption();
}

void Settings::setP2pBindIp(const QString& _bindIp) {
  m_settings.insert("P2pBindIp", _bindIp);
  saveSettings();
}

QString Settings::getP2pBindIp() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  if (!m_cmdLineParser->hasP2pBindIp() && m_settings.contains("P2pBindIp")) {
    return m_settings.value("P2pBindIp").toString();
  }
  return m_cmdLineParser->getP2pBindIp();
}

void Settings::setP2pBindPort(const quint16& _bindPort) {
  m_settings.insert("P2pBindPort", _bindPort);
  saveSettings();
}

quint16 Settings::getP2pBindPort() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  if (!m_cmdLineParser->hasP2pBindPort() && m_settings.contains("P2pBindPort")) {
    return m_settings.value("P2pBindPort").toVariant().toInt();
  }
  return m_cmdLineParser->getP2pBindPort();
}

void Settings::setP2pExternalPort(const quint16& _externalPort) {
  m_settings.insert("P2pExternalPort", _externalPort);
  saveSettings();
}

quint16 Settings::getP2pExternalPort() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  if (!m_cmdLineParser->hasP2pExternalPort() && m_settings.contains("P2pExternalPort")) {
    return m_settings.value("P2pExternalPort").toVariant().toInt();
  }
  return m_cmdLineParser->getP2pExternalPort();
}

bool Settings::hasP2pBindIp() const {
  return m_cmdLineParser->hasP2pBindIp();
}

bool Settings::hasP2pBindPort() const {
  return m_cmdLineParser->hasP2pBindPort();
}

bool Settings::hasP2pExternalPort() const {
  return m_cmdLineParser->hasP2pExternalPort();
}

QStringList Settings::getPeers() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getPeers();
}

QStringList Settings::getPriorityNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getPiorityNodes();
}

QStringList Settings::getExclusiveNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getExclusiveNodes();
}

QStringList Settings::getSeedNodes() const {
  Q_ASSERT(m_cmdLineParser != nullptr);
  return m_cmdLineParser->getSeedNodes();
}

QDir Settings::getDataDir() const {
  Q_CHECK_PTR(m_cmdLineParser);
  return QDir(m_cmdLineParser->getDataDir());
}

QString Settings::getWalletFile() const {
  return m_settings.contains("walletFile") ? m_settings.value("walletFile").toString() : QString();
}

QStringList Settings::getRecentWalletsList() const {
   QStringList recent_wallets;
   if (m_settings.contains("recentWallets")) {
     recent_wallets << m_settings.value("recentWallets").toVariant().toStringList();
   }
   return recent_wallets;
}

QString Settings::getAddressBookFile() const {
  QString addressBookFile;
  if (isGlobalAddressBookEnabled()) {
    addressBookFile = getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".addressbook");
  } else {
    addressBookFile = m_settings.value("walletFile").toString();
    addressBookFile.replace(addressBookFile.lastIndexOf(".wallet"), 7, ".addressbook");
  } 
  return addressBookFile;
}

bool Settings::isEncrypted() const {
  return m_settings.contains("encrypted") ? m_settings.value("encrypted").toBool() : false;
}

QString Settings::getVersion() const {
  static QString version;
  if (version.isEmpty()) {
    version = CN_PROJECT_VERSION;
    QString revision = CN_WALLET_REV;
    QString commit = BUILD_COMMIT_ID;
    if (commit != "") version.append("-").append(commit);
    if (revision != "") version.append(revision);
  }
  return version;
}

bool Settings::isStartOnLoginEnabled() const {
  bool res = false;
#ifdef Q_OS_MAC
  QDir autorunDir = QDir::home();
  if (!autorunDir.cd("Library") || !autorunDir.cd("LaunchAgents")) {
    return false;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".plist");
  if (!QFile::exists(autorunFilePath)) {
    return false;
  }

  QSettings autorunSettings(autorunFilePath, QSettings::NativeFormat);
  res = autorunSettings.value("RunAtLoad", false).toBool();
#elif defined(Q_OS_LINUX)
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (configPath.isEmpty()) {
    return false;
  }

  QDir autorunDir(configPath);
  if (!autorunDir.cd("autostart")) {
    return false;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".desktop");
  res = QFile::exists(autorunFilePath);
#elif defined(Q_OS_WIN)
  QSettings autorunSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString keyName = QString("%1Wallet").arg(CurrencyAdapter::instance().getCurrencyDisplayName());
  res = autorunSettings.contains(keyName) &&
    !QDir::fromNativeSeparators(autorunSettings.value(keyName).toString()).compare(QApplication::applicationFilePath());
#endif
  return res;
}

#ifdef Q_OS_WIN
bool Settings::isMinimizeToTrayEnabled() const {
  return m_settings.contains("minimizeToTray") ? m_settings.value("minimizeToTray").toBool() : false;
}

bool Settings::isCloseToTrayEnabled() const {
  return m_settings.contains("closeToTray") ? m_settings.value("closeToTray").toBool() : false;
}
#endif

void Settings::setWalletFile(const QString& _file) {
  if (_file.endsWith(".wallet") || _file.endsWith(".keys")) {
    m_settings.insert("walletFile", _file);
  } else {
    m_settings.insert("walletFile", _file + ".wallet");
  }

  QStringList recentWallets;
  QString file = m_settings.value("walletFile").toString();
  if (m_settings.contains("recentWallets")) {
    recentWallets = m_settings.value("recentWallets").toVariant().toStringList();
    foreach (const QString &recentFile, recentWallets) {
      if (recentFile.contains(file)) {
        recentWallets.removeOne(recentFile);
      }
    }
  }
  recentWallets.prepend(file);
  while (recentWallets.size() > maxRecentFiles) {
    recentWallets.removeLast();
  }

  m_settings.insert("recentWallets", QJsonArray::fromStringList(recentWallets));

  saveSettings();
}

void Settings::setEncrypted(bool _encrypted) {
  if (isEncrypted() != _encrypted) {
    m_settings.insert("encrypted", _encrypted);
    saveSettings();
  }
}

void Settings::setCurrentTheme(const QString& _theme) {
}

void Settings::setStartOnLoginEnabled(bool _enable) {
#ifdef Q_OS_MAC
  QDir autorunDir = QDir::home();
  if (!autorunDir.cd("Library") || !autorunDir.cd("LaunchAgents")) {
    return;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".plist");
  QSettings autorunSettings(autorunFilePath, QSettings::NativeFormat);
  autorunSettings.setValue("Label", "org." + QCoreApplication::applicationName());
  autorunSettings.setValue("Program", QApplication::applicationFilePath());
  autorunSettings.setValue("RunAtLoad", _enable);
  autorunSettings.setValue("ProcessType", "InterActive");
#elif defined(Q_OS_LINUX)
  QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  if (configPath.isEmpty()) {
    return;
  }

  QDir autorunDir(configPath);
  if(!autorunDir.exists("autostart")) {
    autorunDir.mkdir("autostart");
  }

  if (!autorunDir.cd("autostart")) {
    return;
  }

  QString autorunFilePath = autorunDir.absoluteFilePath(QCoreApplication::applicationName() + ".desktop");
  QFile autorunFile(autorunFilePath);
  if (!autorunFile.open(QFile::WriteOnly | QFile::Truncate)) {
    return;
  }

  if (_enable) {
    autorunFile.write("[Desktop Entry]\n");
    autorunFile.write("Type=Application\n");
    autorunFile.write(QString("Name=%1 Wallet\n").arg(CurrencyAdapter::instance().getCurrencyDisplayName()).toLocal8Bit());
    autorunFile.write(QString("Exec=%1\n").arg(QApplication::applicationFilePath()).toLocal8Bit());
    autorunFile.write("Terminal=false\n");
    autorunFile.write("Hidden=false\n");
    autorunFile.close();
  } else {
    QFile::remove(autorunFilePath);
  }
#elif defined(Q_OS_WIN)
  QSettings autorunSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  QString keyName = QString("%1Wallet").arg(CurrencyAdapter::instance().getCurrencyDisplayName());
  if (_enable) {
    autorunSettings.setValue(keyName, QDir::toNativeSeparators(QApplication::applicationFilePath()));
  } else {
    autorunSettings.remove(keyName);
  }
#endif
}

#ifdef Q_OS_WIN
void Settings::setMinimizeToTrayEnabled(bool _enable) {
  if (isMinimizeToTrayEnabled() != _enable) {
    m_settings.insert("minimizeToTray", _enable);
    saveSettings();
  }
}

void Settings::setCloseToTrayEnabled(bool _enable) {
  if (isCloseToTrayEnabled() != _enable) {
    m_settings.insert("closeToTray", _enable);
    saveSettings();
  }
}
#endif

void Settings::saveSettings() const {
  QFile cfgFile(getDataDir().absoluteFilePath(QCoreApplication::applicationName() + ".cfg"));
  if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    QJsonDocument cfg_doc(m_settings);
    cfgFile.write(cfg_doc.toJson());
    cfgFile.close();
  }
}

void Settings::setConnection(const QString& _connection) {
  m_settings.insert("connectionMode", _connection);
  saveSettings();
}

QString Settings::getConnection() const {
  QString connection = "auto";
  if (m_settings.contains("connectionMode")) {
    connection = m_settings.value("connectionMode").toString();
  }
  return connection;
}

void Settings::setLocalDaemonPort(const quint16& _daemonPort) {
  m_settings.insert("daemonPort", _daemonPort);
  saveSettings();
}

quint16 Settings::getLocalDaemonPort() const {
  quint16 port = 0;
  if (m_settings.contains("daemonPort")) {
    port = m_settings.value("daemonPort").toVariant().toInt();
  }
  if (port == 0) port = DynexCN::RPC_DEFAULT_PORT;
  return port;
}

void Settings::setRemoteNode(const QString& _remoteNode) {
  m_settings.insert("remoteNode", _remoteNode);
  saveSettings();
}

QString Settings::getRemoteNode() const {
  QString node;
  if (m_settings.contains("remoteNode")) {
    node = m_settings.value("remoteNode").toString();
  }
  return node;
}

bool Settings::isGlobalAddressBookEnabled() const {
  return m_settings.contains("globalAddressBook") ? m_settings.value("globalAddressBook").toBool() : false;
}

void Settings::setGlobalAddressBookEnabled(bool _enable) {
  if (isGlobalAddressBookEnabled() != _enable) {
    m_settings.insert("globalAddressBook", _enable);
    saveSettings();
  }
}

quint16 Settings::getLogLevel() const {
  return m_cmdLineParser->getLogLevel();
}

}
