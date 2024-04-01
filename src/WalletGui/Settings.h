// Copyright (c) 2021-2023, Dynex Developers
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
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers

#pragma once

#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

namespace WalletGui {

const int maxRecentFiles = 16;

class CommandLineParser;

class Settings : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Settings)

public:
  static Settings& instance();

  void setCommandLineParser(CommandLineParser* _cmd_line_parser);
  void load();

  bool hasAllowLocalIpOption() const;
  bool hasHideMyPortOption() const;
  bool isTestnet() const;
  QDir getDataDir() const;
  void setP2pBindIp(const QString& _bindIp);
  QString getP2pBindIp() const;
  void setP2pBindPort(const quint16& _bindPort);
  quint16 getP2pBindPort() const;
  void setP2pExternalPort(const quint16& _externalPort);
  quint16 getP2pExternalPort() const;
  bool hasP2pBindIp() const;
  bool hasP2pBindPort() const;
  bool hasP2pExternalPort() const;

  QStringList getExclusiveNodes() const;
  QStringList getPeers() const;
  QStringList getPriorityNodes() const;
  QStringList getSeedNodes() const;

  QString getWalletFile() const;
  QStringList getRecentWalletsList() const;
  QString getAddressBookFile() const;
  bool isEncrypted() const;
  QString getVersion() const;
  bool isStartOnLoginEnabled() const;
#ifdef Q_OS_WIN
  bool isMinimizeToTrayEnabled() const;
  bool isCloseToTrayEnabled() const;
#endif

  void setWalletFile(const QString& _file);
  void setEncrypted(bool _encrypted);
  void setCurrentTheme(const QString& _theme);
  void setStartOnLoginEnabled(bool _enable);
#ifdef Q_OS_WIN
  void setMinimizeToTrayEnabled(bool _enable);
  void setCloseToTrayEnabled(bool _enable);
#endif

  void setConnection(const QString& _connection);
  QString getConnection() const;
  void setLocalDaemonPort(const quint16& _daemonPort);
  quint16 getLocalDaemonPort() const;
  void setRemoteNode(const QString& _remoteNode);
  QString getRemoteNode() const;

  bool isGlobalAddressBookEnabled() const;
  void setGlobalAddressBookEnabled(bool _enable);

  quint16 getLogLevel() const;

private:
  QJsonObject m_settings;
  CommandLineParser* m_cmdLineParser;

  Settings();
  ~Settings();

  void saveSettings() const;
};

}
