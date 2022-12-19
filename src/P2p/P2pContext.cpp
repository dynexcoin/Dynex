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


#include "P2pContext.h"

#include <System/EventLock.h>
#include <System/InterruptedException.h>
#include <System/Ipv4Address.h>
#include <System/OperationTimeout.h>

#include "LevinProtocol.h"

using namespace System;

namespace CryptoNote {

P2pContext::Message::Message(P2pMessage&& msg, Type messageType, uint32_t returnCode) :
  messageType(messageType), returnCode(returnCode) {
  type = msg.type;
  data = std::move(msg.data);
}

size_t P2pContext::Message::size() const {
  return data.size();
}

P2pContext::P2pContext(
  Dispatcher& dispatcher,
  TcpConnection&& conn,
  bool isIncoming,
  const NetworkAddress& remoteAddress,
  std::chrono::nanoseconds timedSyncInterval,
  const CORE_SYNC_DATA& timedSyncData)
  :
  incoming(isIncoming),
  remoteAddress(remoteAddress),
  dispatcher(dispatcher),
  contextGroup(dispatcher),
  timeStarted(Clock::now()),
  timedSyncInterval(timedSyncInterval),
  timedSyncData(timedSyncData),
  timedSyncTimer(dispatcher),
  timedSyncFinished(dispatcher),
  connection(std::move(conn)),
  writeEvent(dispatcher),
  readEvent(dispatcher) {
  writeEvent.set();
  readEvent.set();
  lastReadTime = timeStarted; // use current time
  contextGroup.spawn(std::bind(&P2pContext::timedSyncLoop, this));
}

P2pContext::~P2pContext() {
  stop();
  // wait for timedSyncLoop finish
  timedSyncFinished.wait();
  // ensure that all read/write operations completed
  readEvent.wait();
  writeEvent.wait();
}

PeerIdType P2pContext::getPeerId() const {
  return peerId;
}

uint16_t P2pContext::getPeerPort() const {
  return peerPort;
}

const NetworkAddress& P2pContext::getRemoteAddress() const {
  return remoteAddress;
}

bool P2pContext::isIncoming() const {
  return incoming;
}

void P2pContext::setPeerInfo(uint8_t protocolVersion, PeerIdType id, uint16_t port) {
  version = protocolVersion;
  peerId = id;
  if (isIncoming()) {
    peerPort = port;
  }
}

bool P2pContext::readCommand(LevinProtocol::Command& cmd) {
  if (stopped) {
    throw InterruptedException();
  }

  EventLock lk(readEvent);
  bool result = LevinProtocol(connection).readCommand(cmd);
  lastReadTime = Clock::now();
  return result;
}

void P2pContext::writeMessage(const Message& msg) {
  if (stopped) {
    throw InterruptedException();
  }

  EventLock lk(writeEvent);
  LevinProtocol proto(connection);

  switch (msg.messageType) {
  case P2pContext::Message::NOTIFY:
    proto.sendMessage(msg.type, msg.data, false);
    break;
  case P2pContext::Message::REQUEST:
    proto.sendMessage(msg.type, msg.data, true);
    break;
  case P2pContext::Message::REPLY:
    proto.sendReply(msg.type, msg.data, msg.returnCode);
    break;
  }
}

void P2pContext::start() {
  // stub for OperationTimeout class
} 

void P2pContext::stop() {
  if (!stopped) {
    stopped = true;
    contextGroup.interrupt();
  }
}

void P2pContext::timedSyncLoop() {
  // construct message
  P2pContext::Message timedSyncMessage{ 
    P2pMessage{ 
      COMMAND_TIMED_SYNC::ID, 
      LevinProtocol::encode(COMMAND_TIMED_SYNC::request{ timedSyncData })
    }, 
    P2pContext::Message::REQUEST 
  };

  while (!stopped) {
    try {
      timedSyncTimer.sleep(timedSyncInterval);

      OperationTimeout<P2pContext> timeout(dispatcher, *this, timedSyncInterval);
      writeMessage(timedSyncMessage);

      // check if we had read operation in given time interval
      if ((lastReadTime + timedSyncInterval * 2) < Clock::now()) {
        stop();
        break;
      }
    } catch (InterruptedException&) {
      // someone stopped us
    } catch (std::exception&) {
      stop(); // stop connection on write error
      break;
    }
  }

  timedSyncFinished.set();
}

P2pContext::Message makeReply(uint32_t command, const BinaryArray& data, uint32_t returnCode) {
  return P2pContext::Message(
    P2pMessage{ command, data },
    P2pContext::Message::REPLY,
    returnCode);
}

P2pContext::Message makeRequest(uint32_t command, const BinaryArray& data) {
  return P2pContext::Message(
    P2pMessage{ command, data },
    P2pContext::Message::REQUEST);
}

std::ostream& operator <<(std::ostream& s, const P2pContext& conn) {
  return s << "[" << conn.getRemoteAddress() << "]";
}

}
