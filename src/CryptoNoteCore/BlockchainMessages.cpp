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


#include "CryptoNoteCore/BlockchainMessages.h"

namespace CryptoNote {

NewBlockMessage::NewBlockMessage(const Crypto::Hash& hash) : blockHash(hash) {}

void NewBlockMessage::get(Crypto::Hash& hash) const {
  hash = blockHash;
}

NewAlternativeBlockMessage::NewAlternativeBlockMessage(const Crypto::Hash& hash) : blockHash(hash) {}

void NewAlternativeBlockMessage::get(Crypto::Hash& hash) const {
  hash = blockHash;
}

ChainSwitchMessage::ChainSwitchMessage(std::vector<Crypto::Hash>&& hashes) : blocksFromCommonRoot(std::move(hashes)) {}

ChainSwitchMessage::ChainSwitchMessage(const ChainSwitchMessage& other) : blocksFromCommonRoot(other.blocksFromCommonRoot) {}

void ChainSwitchMessage::get(std::vector<Crypto::Hash>& hashes) const {
  hashes = blocksFromCommonRoot;
}

BlockchainMessage::BlockchainMessage(NewBlockMessage&& message) : type(MessageType::NEW_BLOCK_MESSAGE), newBlockMessage(std::move(message)) {}

BlockchainMessage::BlockchainMessage(NewAlternativeBlockMessage&& message) : type(MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE), newAlternativeBlockMessage(std::move(message)) {}

BlockchainMessage::BlockchainMessage(ChainSwitchMessage&& message) : type(MessageType::CHAIN_SWITCH_MESSAGE) {
	chainSwitchMessage = new ChainSwitchMessage(std::move(message));
}

BlockchainMessage::BlockchainMessage(const BlockchainMessage& other) : type(other.type) {
  switch (type) {
    case MessageType::NEW_BLOCK_MESSAGE:
      new (&newBlockMessage) NewBlockMessage(other.newBlockMessage);
      break;
    case MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE:
      new (&newAlternativeBlockMessage) NewAlternativeBlockMessage(other.newAlternativeBlockMessage);
      break;
    case MessageType::CHAIN_SWITCH_MESSAGE:
	  chainSwitchMessage = new ChainSwitchMessage(*other.chainSwitchMessage);
      break;
  }
}

BlockchainMessage::~BlockchainMessage() {
  switch (type) {
    case MessageType::NEW_BLOCK_MESSAGE:
      newBlockMessage.~NewBlockMessage();
      break;
    case MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE:
      newAlternativeBlockMessage.~NewAlternativeBlockMessage();
      break;
    case MessageType::CHAIN_SWITCH_MESSAGE:
	  delete chainSwitchMessage;
      break;
  }
}

BlockchainMessage::MessageType BlockchainMessage::getType() const {
  return type;
}

bool BlockchainMessage::getNewBlockHash(Crypto::Hash& hash) const {
  if (type == MessageType::NEW_BLOCK_MESSAGE) {
    newBlockMessage.get(hash);
    return true;
  } else {
    return false;
  }
}

bool BlockchainMessage::getNewAlternativeBlockHash(Crypto::Hash& hash) const {
  if (type == MessageType::NEW_ALTERNATIVE_BLOCK_MESSAGE) {
    newAlternativeBlockMessage.get(hash);
    return true;
  } else {
    return false;
  }
}

bool BlockchainMessage::getChainSwitch(std::vector<Crypto::Hash>& hashes) const {
  if (type == MessageType::CHAIN_SWITCH_MESSAGE) {
    chainSwitchMessage->get(hashes);
    return true;
  } else {
    return false;
  }
}

}
