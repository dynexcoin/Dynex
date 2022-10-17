// Copyright (c) 2021-2022, Dynex Developers
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
// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#include "P2pConnectionProxy.h"

#include "LevinProtocol.h"
#include "P2pContext.h"
#include "P2pNode.h"

#include <System/InterruptedException.h>

using namespace System;

namespace CryptoNote {

P2pConnectionProxy::P2pConnectionProxy(P2pContextOwner&& ctx, IP2pNodeInternal& node)
  : m_contextOwner(std::move(ctx)), m_context(m_contextOwner.get()), m_node(node) {}

P2pConnectionProxy::~P2pConnectionProxy() {
  m_context.stop();
}

bool P2pConnectionProxy::processIncomingHandshake() {
  LevinProtocol::Command cmd;
  if (!m_context.readCommand(cmd)) {
    throw std::runtime_error("Connection unexpectedly closed");
  }

  if (cmd.command == COMMAND_HANDSHAKE::ID) {
    handleHandshakeRequest(cmd);
    return true;
  } 
  
  if (cmd.command == COMMAND_PING::ID) {
    COMMAND_PING::response resp{ PING_OK_RESPONSE_STATUS_TEXT, m_node.getPeerId() };
    m_context.writeMessage(makeReply(COMMAND_PING::ID, LevinProtocol::encode(resp), LEVIN_PROTOCOL_RETCODE_SUCCESS));
    return false;
  } 

  throw std::runtime_error("Unexpected command: " + std::to_string(cmd.command));
}

void P2pConnectionProxy::read(P2pMessage& message) {
  if (!m_readQueue.empty()) {
    message = std::move(m_readQueue.front());
    m_readQueue.pop();
    return;
  }

  for (;;) {
    LevinProtocol::Command cmd;
    if (!m_context.readCommand(cmd)) {
      throw InterruptedException();
    }

    message.type = cmd.command;

    if (cmd.command == COMMAND_HANDSHAKE::ID) {
      handleHandshakeResponse(cmd, message);
      break;
    } else if (cmd.command == COMMAND_TIMED_SYNC::ID) {
      handleTimedSync(cmd);
    } else {
      message.data = std::move(cmd.buf);
      break;
    }
  }
}

void P2pConnectionProxy::write(const P2pMessage &message) {
  if (message.type == COMMAND_HANDSHAKE::ID) {
    writeHandshake(message);
  } else {
    m_context.writeMessage(P2pContext::Message(P2pMessage(message), P2pContext::Message::NOTIFY));
  }
}

void P2pConnectionProxy::ban() {
  // not implemented
}

void P2pConnectionProxy::stop() {
  m_context.stop();
}

void P2pConnectionProxy::writeHandshake(const P2pMessage &message) {
  CORE_SYNC_DATA coreSync;
  LevinProtocol::decode(message.data, coreSync);

  if (m_context.isIncoming()) {
    // response
    COMMAND_HANDSHAKE::response res;
    res.node_data = m_node.getNodeData();
    res.payload_data = coreSync;
    res.local_peerlist = m_node.getLocalPeerList();
    m_context.writeMessage(makeReply(COMMAND_HANDSHAKE::ID, LevinProtocol::encode(res), LEVIN_PROTOCOL_RETCODE_SUCCESS));
    m_node.tryPing(m_context);
  } else {
    // request
    COMMAND_HANDSHAKE::request req;
    req.node_data = m_node.getNodeData();
    req.payload_data = coreSync;
    m_context.writeMessage(makeRequest(COMMAND_HANDSHAKE::ID, LevinProtocol::encode(req)));
  }
}

void P2pConnectionProxy::handleHandshakeRequest(const LevinProtocol::Command& cmd) {
  COMMAND_HANDSHAKE::request req;
  if (!LevinProtocol::decode<COMMAND_HANDSHAKE::request>(cmd.buf, req)) {
    throw std::runtime_error("Failed to decode COMMAND_HANDSHAKE request");
  }

  m_node.handleNodeData(req.node_data, m_context);
  m_readQueue.push(P2pMessage{ 
    cmd.command, LevinProtocol::encode(req.payload_data) }); // enqueue payload info
}

void P2pConnectionProxy::handleHandshakeResponse(const LevinProtocol::Command& cmd, P2pMessage& message) {
  if (m_context.isIncoming()) {
    // handshake should be already received by P2pNode
    throw std::runtime_error("Unexpected COMMAND_HANDSHAKE from incoming connection");
  }

  COMMAND_HANDSHAKE::response res;
  if (!LevinProtocol::decode(cmd.buf, res)) {
    throw std::runtime_error("Invalid handshake message format");
  }

  m_node.handleNodeData(res.node_data, m_context);
  m_node.handleRemotePeerList(res.local_peerlist, res.node_data.local_time);

  message.data = LevinProtocol::encode(res.payload_data);
}

void P2pConnectionProxy::handleTimedSync(const LevinProtocol::Command& cmd) {
  if (cmd.isResponse) {
    COMMAND_TIMED_SYNC::response res;
    LevinProtocol::decode(cmd.buf, res);
    m_node.handleRemotePeerList(res.local_peerlist, res.local_time);
  } else {
    // we ignore information from the request
    // COMMAND_TIMED_SYNC::request req;
    // LevinProtocol::decode(cmd.buf, req);
    COMMAND_TIMED_SYNC::response res;

    res.local_time = time(nullptr);
    res.local_peerlist = m_node.getLocalPeerList();
    res.payload_data = m_node.getGenesisPayload();

    m_context.writeMessage(makeReply(COMMAND_TIMED_SYNC::ID, LevinProtocol::encode(res), LEVIN_PROTOCOL_RETCODE_SUCCESS));
  }
}

}
