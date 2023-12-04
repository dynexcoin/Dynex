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

#pragma once

#include <QCommandLineParser>
#include <QObject>

namespace WalletGui {

class CommandLineParser : public QObject {
  Q_OBJECT

public:
  CommandLineParser(QObject* _parent);
  ~CommandLineParser();

  bool process(const QStringList& _argv);

  bool hasHelpOption() const;
  bool hasVersionOption() const;
  bool hasTestnetOption() const;
  bool hasMinimizedOption() const;
  bool hasAllowLocalIpOption() const;
  bool hasHideMyPortOption() const;
  QString getErrorText() const;
  QString getHelpText() const;
  QString getP2pBindIp() const;
  quint16 getP2pBindPort() const;
  quint16 getP2pExternalPort() const;
  QStringList getPeers() const;
  QStringList getPiorityNodes() const;
  QStringList getExclusiveNodes() const;
  QStringList getSeedNodes() const;
  QString getDataDir() const;
  bool hasP2pBindIp() const;
  bool hasP2pBindPort() const;
  bool hasP2pExternalPort() const;

private:
  QCommandLineParser m_parser;
  QCommandLineOption m_helpOption;
  QCommandLineOption m_versionOption;
  QCommandLineOption m_testnetOption;
  QCommandLineOption m_p2pBindIpOption;
  QCommandLineOption m_p2pBindPortOption;
  QCommandLineOption m_p2pExternalOption;
  QCommandLineOption m_allowLocalIpOption;
  QCommandLineOption m_addPeerOption;
  QCommandLineOption m_addPriorityNodeOption;
  QCommandLineOption m_addExclusiveNodeOption;
  QCommandLineOption m_seedNodeOption;
  QCommandLineOption m_hideMyPortOption;
  QCommandLineOption m_dataDirOption;
  QCommandLineOption m_minimized;
};

}
