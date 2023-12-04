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

#include <Common/Util.h>
#include <DynexCNConfig.h>

#include "CommandLineParser.h"

namespace WalletGui {

CommandLineParser::CommandLineParser(QObject* _parent) : QObject(_parent), m_parser(), m_helpOption(m_parser.addHelpOption()),
  m_versionOption(m_parser.addVersionOption()),
  m_testnetOption("testnet", tr("Used to deploy test nets. Checkpoints and hardcoded seeds are ignored, network id is changed. "
    "Use it with –data-dir flag. The wallet must be launched with –testnet flag")),
  m_p2pBindIpOption("p2p-bind-ip", tr("Interface for p2p network protocol"), tr("ip"), "0.0.0.0"),
  m_p2pBindPortOption("p2p-bind-port", tr("Port for p2p network protocol"), tr("port"), QString::number(DynexCN::P2P_DEFAULT_PORT)),
  m_p2pExternalOption("p2p-external-port", tr("External port for p2p network protocol (if port forwarding used with NAT)"),
    tr("port"), 0),
  m_allowLocalIpOption("allow-local-ip", tr("Allow local ip add to peer list, mostly in debug purposes")),
  m_addPeerOption("add-peer", tr("Manually add peer to local peerlist"), tr("peer")),
  m_addPriorityNodeOption("add-priority-node", tr("Specify list of peers to connect to and attempt to keep the connection open"),
    tr("node")),
  m_addExclusiveNodeOption("add-exclusive-node", tr("Specify list of peers to connect to only. If this option is given the options "
    "add-priority-node and seed-node are ignored"), tr("node")),
  m_seedNodeOption("seed-node", tr("Connect to a node to retrieve peer addresses, and disconnect"), tr("node")),
  m_hideMyPortOption("hide-my-port", tr("Do not announce yourself as peerlist candidate")),
  m_dataDirOption("data-dir", tr("Specify data directory"), tr("directory"), QString::fromStdString(Tools::getDefaultDataDirectory())),
  m_minimized("minimized", tr("Run application in minimized mode")) {
//m_parser.setApplicationDescription(tr("Dynex wallet"));
  m_parser.addOption(m_testnetOption);
  m_parser.addOption(m_p2pBindIpOption);
  m_parser.addOption(m_p2pBindPortOption);
  m_parser.addOption(m_p2pExternalOption);
  m_parser.addOption(m_allowLocalIpOption);
  m_parser.addOption(m_addPeerOption);
  m_parser.addOption(m_addPriorityNodeOption);
  m_parser.addOption(m_addExclusiveNodeOption);
  m_parser.addOption(m_seedNodeOption);
  m_parser.addOption(m_hideMyPortOption);
  m_parser.addOption(m_dataDirOption);
  m_parser.addOption(m_minimized);
}

CommandLineParser::~CommandLineParser() {
}

bool CommandLineParser::process(const QStringList& _argv) {
#ifdef Q_OS_WIN
  return m_parser.parse(_argv);
#else
  m_parser.process(_argv);
  return true;
#endif
}

bool CommandLineParser::hasHelpOption() const {
  return m_parser.isSet(m_helpOption);
}

bool CommandLineParser::hasMinimizedOption() const {
  return m_parser.isSet(m_minimized);
}

bool CommandLineParser::hasVersionOption() const {
  return m_parser.isSet(m_versionOption);
}

bool CommandLineParser::hasTestnetOption() const {
  return m_parser.isSet(m_testnetOption);
}

bool CommandLineParser::hasAllowLocalIpOption() const {
  return m_parser.isSet(m_allowLocalIpOption);
}

bool CommandLineParser::hasHideMyPortOption() const {
  return m_parser.isSet(m_hideMyPortOption);
}

QString CommandLineParser::getErrorText() const {
  return m_parser.errorText();
}

QString CommandLineParser::getHelpText() const {
  return m_parser.helpText();
}

QString CommandLineParser::getP2pBindIp() const {
  return m_parser.value(m_p2pBindIpOption);
}

quint16 CommandLineParser::getP2pBindPort() const {
  return m_parser.value(m_p2pBindPortOption).toUShort();
}

quint16 CommandLineParser::getP2pExternalPort() const {
  return m_parser.value(m_p2pExternalOption).toUShort();
}

QStringList CommandLineParser::getPeers() const {
  return m_parser.values(m_addPeerOption);
}

QStringList CommandLineParser::getPiorityNodes() const {
  return m_parser.values(m_addPriorityNodeOption);
}

QStringList CommandLineParser::getExclusiveNodes() const {
  return m_parser.values(m_addExclusiveNodeOption);
}

QStringList CommandLineParser::getSeedNodes() const {
  return m_parser.values(m_seedNodeOption);
}

QString CommandLineParser::getDataDir() const {
  return m_parser.value(m_dataDirOption);
}

bool CommandLineParser::hasP2pBindIp() const {
  return m_parser.isSet(m_p2pBindIpOption);
}

bool CommandLineParser::hasP2pBindPort() const {
  return m_parser.isSet(m_p2pBindPortOption);
}

bool CommandLineParser::hasP2pExternalPort() const {
  return m_parser.isSet(m_p2pExternalOption);
}

}
