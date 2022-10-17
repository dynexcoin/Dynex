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


#pragma once

#include <list>
#include "CryptoNoteCore/CryptoNoteBasic.h"

// ISerializer-based serialization
#include "Serialization/ISerializer.h"
#include "Serialization/SerializationOverloads.h"
#include "CryptoNoteCore/CryptoNoteSerialization.h"

namespace CryptoNote
{

#define BC_COMMANDS_POOL_BASE 2000

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_complete_entry
  {
    std::string block;
    std::vector<std::string> txs;

    void serialize(ISerializer& s) {
      KV_MEMBER(block);
      KV_MEMBER(txs);
    }

  };

  struct BlockFullInfo : public block_complete_entry
  {
    Crypto::Hash block_id;

    void serialize(ISerializer& s) {
      KV_MEMBER(block_id);
      KV_MEMBER(block);
      KV_MEMBER(txs);
    }
  };

  struct TransactionPrefixInfo {
    Crypto::Hash txHash;
    TransactionPrefix txPrefix;

    void serialize(ISerializer& s) {
      KV_MEMBER(txHash);
      KV_MEMBER(txPrefix);
    }
  };

  struct BlockShortInfo {
    Crypto::Hash blockId;
    std::string block;
    std::vector<TransactionPrefixInfo> txPrefixes;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockId);
      KV_MEMBER(block);
      KV_MEMBER(txPrefixes);
    }
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_BLOCK_request
  {
    block_complete_entry b;
    uint32_t current_blockchain_height;
    uint32_t hop;

    void serialize(ISerializer& s) {
      KV_MEMBER(b)
      KV_MEMBER(current_blockchain_height)
      KV_MEMBER(hop)
    }
  };

  struct NOTIFY_NEW_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 1;
    typedef NOTIFY_NEW_BLOCK_request request;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_TRANSACTIONS_request
  {
    std::vector<std::string> txs;

    void serialize(ISerializer& s) {
      KV_MEMBER(txs);
    }

  };

  struct NOTIFY_NEW_TRANSACTIONS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 2;
    typedef NOTIFY_NEW_TRANSACTIONS_request request;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_GET_OBJECTS_request
  {
    std::vector<Crypto::Hash> txs;
    std::vector<Crypto::Hash> blocks;

    void serialize(ISerializer& s) {
      serializeAsBinary(txs, "txs", s);
      serializeAsBinary(blocks, "blocks", s);
    }
  };

  struct NOTIFY_REQUEST_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 3;
    typedef NOTIFY_REQUEST_GET_OBJECTS_request request;
  };

  struct NOTIFY_RESPONSE_GET_OBJECTS_request
  {
    std::vector<std::string> txs;
    std::vector<block_complete_entry> blocks;
    std::vector<Crypto::Hash> missed_ids;
    uint32_t current_blockchain_height;

    void serialize(ISerializer& s) {
      KV_MEMBER(txs)
      KV_MEMBER(blocks)
      serializeAsBinary(missed_ids, "missed_ids", s);
      KV_MEMBER(current_blockchain_height)
    }

  };

  struct NOTIFY_RESPONSE_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 4;
    typedef NOTIFY_RESPONSE_GET_OBJECTS_request request;
  };

  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request
    {
      std::vector<Crypto::Hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

      void serialize(ISerializer& s) {
        serializeAsBinary(block_ids, "block_ids", s);
      }
    };
  };

  struct NOTIFY_RESPONSE_CHAIN_ENTRY_request
  {
    uint32_t start_height;
    uint32_t total_height;
    std::vector<Crypto::Hash> m_block_ids;

    void serialize(ISerializer& s) {
      KV_MEMBER(start_height)
      KV_MEMBER(total_height)
      serializeAsBinary(m_block_ids, "m_block_ids", s);
    }
  };

  struct NOTIFY_RESPONSE_CHAIN_ENTRY
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 7;
    typedef NOTIFY_RESPONSE_CHAIN_ENTRY_request request;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_TX_POOL_request {
    std::vector<Crypto::Hash> txs;

    void serialize(ISerializer& s) {
      serializeAsBinary(txs, "txs", s);
    }
  };

  struct NOTIFY_REQUEST_TX_POOL {
    const static int ID = BC_COMMANDS_POOL_BASE + 8;
    typedef NOTIFY_REQUEST_TX_POOL_request request;
  };
}
