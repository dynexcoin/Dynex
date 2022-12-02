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

#include "CryptoNoteProtocol/CryptoNoteProtocolDefinitions.h"
#include "CryptoNoteCore/CryptoNoteBasic.h"
#include "CryptoNoteCore/Difficulty.h"
#include "crypto/hash.h"

#include "Serialization/SerializationOverloads.h"
#include "Serialization/BlockchainExplorerDataSerialization.h"

namespace CryptoNote {
//-----------------------------------------------
#define CORE_RPC_STATUS_OK "OK"
#define CORE_RPC_STATUS_BUSY "BUSY"

struct EMPTY_STRUCT {
  void serialize(ISerializer &s) {}
};

struct STATUS_STRUCT {
  std::string status;

  void serialize(ISerializer &s) {
    KV_MEMBER(status)
  }
};

struct COMMAND_RPC_GET_HEIGHT {
  typedef EMPTY_STRUCT request;

  struct response {
    uint32_t height;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_BLOCKS_FAST {

  struct request {
    std::vector<Crypto::Hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
    
    void serialize(ISerializer &s) {
      serializeAsBinary(block_ids, "block_ids", s);
    }
  };

  struct response {
    std::vector<block_complete_entry> blocks;
    uint32_t start_height;
    uint32_t current_height;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(blocks)
      KV_MEMBER(start_height)
      KV_MEMBER(current_height)
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_TRANSACTIONS {
  struct request {
    std::vector<std::string> txs_hashes;

    void serialize(ISerializer &s) {
      KV_MEMBER(txs_hashes)
    }
  };

  struct response {
    std::vector<std::string> txs_as_hex; //transactions blobs as hex
    std::vector<std::string> missed_tx;  //not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(txs_as_hex)
      KV_MEMBER(missed_tx)
      KV_MEMBER(status)    
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_POOL_CHANGES {
  struct request {
    Crypto::Hash tailBlockId;
    std::vector<Crypto::Hash> knownTxsIds;

    void serialize(ISerializer &s) {
      KV_MEMBER(tailBlockId)
      serializeAsBinary(knownTxsIds, "knownTxsIds", s);
    }
  };

  struct response {
    bool isTailBlockActual;
    std::vector<BinaryArray> addedTxs;          // Added transactions blobs
    std::vector<Crypto::Hash> deletedTxsIds; // IDs of not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(isTailBlockActual)
      KV_MEMBER(addedTxs)
      serializeAsBinary(deletedTxsIds, "deletedTxsIds", s);
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_POOL_CHANGES_LITE {
  struct request {
    Crypto::Hash tailBlockId;
    std::vector<Crypto::Hash> knownTxsIds;

    void serialize(ISerializer &s) {
      KV_MEMBER(tailBlockId)
      serializeAsBinary(knownTxsIds, "knownTxsIds", s);
    }
  };

  struct response {
    bool isTailBlockActual;
    std::vector<TransactionPrefixInfo> addedTxs;          // Added transactions blobs
    std::vector<Crypto::Hash> deletedTxsIds; // IDs of not found transactions
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(isTailBlockActual)
      KV_MEMBER(addedTxs)
      serializeAsBinary(deletedTxsIds, "deletedTxsIds", s);
      KV_MEMBER(status)
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES {
  
  struct request {
    Crypto::Hash txid;

    void serialize(ISerializer &s) {
      KV_MEMBER(txid)
    }
  };

  struct response {
    std::vector<uint64_t> o_indexes;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(o_indexes)
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request {
  std::vector<uint64_t> amounts;
  uint64_t outs_count;

  void serialize(ISerializer &s) {
    KV_MEMBER(amounts)
    KV_MEMBER(outs_count)
  }
};

#pragma pack(push, 1)
struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry {
  uint64_t global_amount_index;
  Crypto::PublicKey out_key;
};
#pragma pack(pop)

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount {
  uint64_t amount;
  std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry> outs;

  void serialize(ISerializer &s) {
    KV_MEMBER(amount)
    serializeAsBinary(outs, "outs", s);
  }
};

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response {
  std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount> outs;
  std::string status;

  void serialize(ISerializer &s) {
    KV_MEMBER(outs);
    KV_MEMBER(status)
  }
};

struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS {
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request request;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response response;

  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_out_entry out_entry;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_outs_for_amount outs_for_amount;
};

//-----------------------------------------------
struct COMMAND_RPC_SEND_RAW_TX {
  struct request {
    std::string tx_as_hex;

    request() {}
    explicit request(const Transaction &);

    void serialize(ISerializer &s) {
      KV_MEMBER(tx_as_hex)
    }
  };

  struct response {
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
    }
  };
};
//-----------------------------------------------
struct COMMAND_RPC_GET_INFO {
  typedef EMPTY_STRUCT request;

  struct response {
    std::string status;
    std::string version;
    std::string version_num;
    std::string version_build;
    std::string version_remark;
    uint32_t height;
    std::string top_block_hash;
    uint64_t difficulty;
    uint64_t cumulative_difficulty;
    uint64_t next_reward;
    uint64_t min_tx_fee;
    std::string readable_tx_fee;
    uint64_t tx_count;
    uint64_t tx_pool_size;
    uint64_t alt_blocks_count;
    uint64_t outgoing_connections_count;
    uint64_t incoming_connections_count;
    uint64_t rpc_connections_count;
    uint64_t white_peerlist_size;
    uint64_t grey_peerlist_size;
    uint32_t last_known_block_index;
    uint64_t start_time;
    std::string fee_address;
    uint8_t block_major_version;
    std::string already_generated_coins;
    std::string contact;   

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(version)
      KV_MEMBER(version_num)      
      KV_MEMBER(version_build)      
      KV_MEMBER(version_remark)      
      KV_MEMBER(height)
      KV_MEMBER(top_block_hash)
      KV_MEMBER(difficulty)
      KV_MEMBER(cumulative_difficulty)
      KV_MEMBER(next_reward)
      KV_MEMBER(min_tx_fee)
      KV_MEMBER(readable_tx_fee)
      KV_MEMBER(tx_count)
      KV_MEMBER(tx_pool_size)
      KV_MEMBER(alt_blocks_count)
      KV_MEMBER(outgoing_connections_count)
      KV_MEMBER(incoming_connections_count)
      KV_MEMBER(rpc_connections_count)
      KV_MEMBER(white_peerlist_size)
      KV_MEMBER(grey_peerlist_size)
      KV_MEMBER(last_known_block_index)
      KV_MEMBER(start_time)
      KV_MEMBER(fee_address)
      KV_MEMBER(block_major_version)
      KV_MEMBER(already_generated_coins)
      KV_MEMBER(contact)      
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_STOP_DAEMON {
  typedef EMPTY_STRUCT request;
  typedef STATUS_STRUCT response;
};

//-----------------------------------------------
struct COMMAND_RPC_GET_PEER_LIST {
	typedef EMPTY_STRUCT request;

	struct response {
		std::vector<std::string> peers;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(peers)
			KV_MEMBER(status)
		}
	};
};

//-----------------------------------------------
struct COMMAND_RPC_GET_FEE_ADDRESS {
  typedef EMPTY_STRUCT request;

  struct response {
    std::string fee_address;
	std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(fee_address)
	  KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GETBLOCKCOUNT {
  typedef std::vector<std::string> request;

  struct response {
    uint64_t count;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(count)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GETBLOCKHASH {
  typedef std::vector<uint64_t> request;
  typedef std::string response;
};

struct COMMAND_RPC_GETBLOCKTEMPLATE {
  struct request {
    uint64_t reserve_size; //max 255 bytes
    std::string wallet_address;

    void serialize(ISerializer &s) {
      KV_MEMBER(reserve_size)
      KV_MEMBER(wallet_address)
    }
  };

  struct response {
    uint64_t difficulty;
    uint32_t height;
    uint64_t reserved_offset;
    std::string blocktemplate_blob;
	std::string blockhashing_blob;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(difficulty)
      KV_MEMBER(height)
      KV_MEMBER(reserved_offset)
      KV_MEMBER(blocktemplate_blob)
	  KV_MEMBER(blockhashing_blob)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_CURRENCY_ID {
  typedef EMPTY_STRUCT request;

  struct response {
    std::string currency_id_blob;

    void serialize(ISerializer &s) {
      KV_MEMBER(currency_id_blob)
    }
  };
};

struct COMMAND_RPC_SUBMITBLOCK {
  typedef std::vector<std::string> request;
  typedef STATUS_STRUCT response;
};

struct block_header_response {
  uint8_t major_version;
  uint8_t minor_version;
  uint64_t timestamp;
  std::string prev_hash;
  uint32_t nonce;
  bool orphan_status;
  uint32_t height;
  uint32_t depth;
  std::string hash;
  difficulty_type difficulty;
  uint64_t reward;

  void serialize(ISerializer &s) {
    KV_MEMBER(major_version)
    KV_MEMBER(minor_version)
    KV_MEMBER(timestamp)
    KV_MEMBER(prev_hash)
    KV_MEMBER(nonce)
    KV_MEMBER(orphan_status)
    KV_MEMBER(height)
    KV_MEMBER(depth)
    KV_MEMBER(hash)
    KV_MEMBER(difficulty)
    KV_MEMBER(reward)
  }
};

struct BLOCK_HEADER_RESPONSE {
  std::string status;
  block_header_response block_header;

  void serialize(ISerializer &s) {
    KV_MEMBER(block_header)
    KV_MEMBER(status)
  }
};


struct f_transaction_short_response {
  std::string hash;
  uint64_t fee;
  uint64_t amount_out;
  uint64_t size;

  void serialize(ISerializer &s) {
    KV_MEMBER(hash)
    KV_MEMBER(fee)
    KV_MEMBER(amount_out)
    KV_MEMBER(size)
  }
};

struct transaction_pool_response {
  std::string hash;
  uint64_t fee;
  uint64_t amount_out;
  uint64_t size;
  uint64_t receiveTime;

  void serialize(ISerializer &s) {
    KV_MEMBER(hash)
    KV_MEMBER(fee)
    KV_MEMBER(amount_out)
    KV_MEMBER(size)
    KV_MEMBER(receiveTime)
  }
};

struct block_short_response {
  uint64_t timestamp;
  uint32_t height;
  std::string hash;
  uint64_t tx_count;
  uint64_t cumul_size;
  difficulty_type difficulty;
  uint64_t min_tx_fee;

  void serialize(ISerializer &s) {
    KV_MEMBER(timestamp)
    KV_MEMBER(height)
    KV_MEMBER(hash)
    KV_MEMBER(cumul_size)
    KV_MEMBER(tx_count)
    KV_MEMBER(difficulty)
    KV_MEMBER(min_tx_fee)
  }
};

struct f_transaction_details_extra_response {
  std::vector<size_t> padding;
  Crypto::PublicKey publicKey; 
  std::vector<std::string> nonce;
  std::vector<uint8_t> raw;

  void serialize(ISerializer &s) {
    KV_MEMBER(padding)
    KV_MEMBER(publicKey)
    KV_MEMBER(nonce)
    KV_MEMBER(raw)
  }
};

struct f_transaction_details_response {
  std::string hash;
  size_t size;
  std::string paymentId;
  uint64_t mixin;
  uint64_t fee;
  uint64_t amount_out;
  uint32_t confirmations = 0;
  f_transaction_details_extra_response extra;

  void serialize(ISerializer &s) {
    KV_MEMBER(hash)
    KV_MEMBER(size)
    KV_MEMBER(paymentId)
    KV_MEMBER(mixin)
    KV_MEMBER(fee)
    KV_MEMBER(amount_out)
    KV_MEMBER(confirmations)
    KV_MEMBER(extra)
  }
};

struct f_mempool_transaction_response {
  std::string hash;
  uint64_t fee;
  uint64_t amount_out;
  uint64_t size;
  uint64_t receiveTime;
  bool keptByBlock;
  uint32_t max_used_block_height;
  std::string max_used_block_id;
  uint32_t last_failed_height;
  std::string last_failed_id;

  void serialize(ISerializer &s) {
    KV_MEMBER(hash)
    KV_MEMBER(fee)
    KV_MEMBER(amount_out)
    KV_MEMBER(size)
	KV_MEMBER(receiveTime)
	KV_MEMBER(keptByBlock)
	KV_MEMBER(max_used_block_height)
	KV_MEMBER(max_used_block_id)
	KV_MEMBER(last_failed_height)
	KV_MEMBER(last_failed_id)
  }
};

struct f_block_details_response {
  uint8_t major_version;
  uint8_t minor_version;  
  uint64_t timestamp;
  std::string prev_hash;
  uint32_t nonce;
  bool orphan_status;
  uint32_t height;
  uint32_t depth;
  std::string hash;
  difficulty_type difficulty;
  difficulty_type cumulativeDifficulty;
  uint64_t reward;
  uint64_t blockSize;
  size_t sizeMedian;
  uint64_t effectiveSizeMedian;
  uint64_t transactionsCumulativeSize;
  std::string alreadyGeneratedCoins;
  uint64_t alreadyGeneratedTransactions;
  uint64_t baseReward;
  double penalty;
  uint64_t totalFeeAmount;
  std::vector<f_transaction_short_response> transactions;

  void serialize(ISerializer &s) {
    KV_MEMBER(major_version)
    KV_MEMBER(minor_version)
    KV_MEMBER(timestamp)
    KV_MEMBER(prev_hash)
    KV_MEMBER(nonce)
    KV_MEMBER(orphan_status)
    KV_MEMBER(height)
    KV_MEMBER(depth)
    KV_MEMBER(hash)
    KV_MEMBER(difficulty)
    KV_MEMBER(cumulativeDifficulty)
    KV_MEMBER(reward)
    KV_MEMBER(blockSize)
    KV_MEMBER(sizeMedian)
    KV_MEMBER(effectiveSizeMedian)
    KV_MEMBER(transactionsCumulativeSize)
    KV_MEMBER(alreadyGeneratedCoins)
    KV_MEMBER(alreadyGeneratedTransactions)
    KV_MEMBER(baseReward)
    KV_MEMBER(penalty)
    KV_MEMBER(totalFeeAmount)
    KV_MEMBER(transactions)
  }
};

struct COMMAND_RPC_GET_LAST_BLOCK_HEADER {
  typedef EMPTY_STRUCT request;
  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH {
  struct request {
    std::string hash;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
    }
  };

  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT {
  struct request {
    uint32_t height;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
    }
  };

  typedef BLOCK_HEADER_RESPONSE response;
};

struct COMMAND_RPC_GET_BLOCKS_LIST {
  struct request {
    uint32_t height;
    uint32_t count = 10;

    void serialize(ISerializer &s) {
      KV_MEMBER(height)
      KV_MEMBER(count)
    }
  };

  struct response {
    std::vector<block_short_response> blocks;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(blocks)
      KV_MEMBER(status)
    }
  };
};

struct F_COMMAND_RPC_GET_BLOCK_DETAILS {
  struct request {
    std::string hash;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
    }
  };

  struct response {
    f_block_details_response block;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(block)
      KV_MEMBER(status)
    }
  };
};

//-----------------------------------------------
struct COMMAND_RPC_GET_TRANSACTIONS_BY_PAYMENT_ID {
	struct request {
		std::string payment_id;

		void serialize(ISerializer &s) {
			KV_MEMBER(payment_id)
		}
	};

	struct response {
		std::vector<f_transaction_short_response> transactions;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(transactions)
				KV_MEMBER(status)
		}
	};
};

struct F_COMMAND_RPC_GET_TRANSACTION_DETAILS {
  struct request {
    std::string hash;

    void serialize(ISerializer &s) {
      KV_MEMBER(hash)
    }
  };

  struct response {
    Transaction tx;
    f_transaction_details_response txDetails;
    block_short_response block;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(tx)
      KV_MEMBER(txDetails)
      KV_MEMBER(block)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTIONS_POOL {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<transaction_pool_response> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactions)
      KV_MEMBER(status)
    }
  };
};

/* Deprecated */
struct F_COMMAND_RPC_GET_POOL {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<f_transaction_short_response> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactions)
      KV_MEMBER(status)
    }
  };
};

/* Deprecated */
struct COMMAND_RPC_GET_MEMPOOL {
  typedef EMPTY_STRUCT request;

  struct response {
    std::vector<f_mempool_transaction_response> mempool;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(mempool)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_QUERY_BLOCKS {
  struct request {
    std::vector<Crypto::Hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
    uint64_t timestamp;

    void serialize(ISerializer &s) {
      serializeAsBinary(block_ids, "block_ids", s);
      KV_MEMBER(timestamp)
    }
  };

  struct response {
    std::string status;
    uint32_t start_height;
    uint32_t current_height;
    uint64_t full_offset;
    std::vector<BlockFullInfo> items;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(start_height)
      KV_MEMBER(current_height)
      KV_MEMBER(full_offset)
      KV_MEMBER(items)
    }
  };
};

struct COMMAND_RPC_QUERY_BLOCKS_LITE {
  struct request {
    std::vector<Crypto::Hash> blockIds;
    uint64_t timestamp;

    void serialize(ISerializer &s) {
      serializeAsBinary(blockIds, "block_ids", s);
      KV_MEMBER(timestamp)
    }
  };

  struct response {
    std::string status;
    uint32_t startHeight;
    uint32_t currentHeight;
    uint64_t fullOffset;
    std::vector<BlockShortInfo> items;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(startHeight)
      KV_MEMBER(currentHeight)
      KV_MEMBER(fullOffset)
      KV_MEMBER(items)
    }
  };
};

struct COMMAND_RPC_GEN_PAYMENT_ID {
  typedef EMPTY_STRUCT request;
  
  struct response {
	  std::string payment_id;

	  void serialize(ISerializer &s) {
		  KV_MEMBER(payment_id)
	  }
  };
};

//-----------------------------------------------
struct K_COMMAND_RPC_CHECK_TX_KEY {
	struct request {
		std::string txid;
		std::string txkey;
		std::string address;

		void serialize(ISerializer &s) {
			KV_MEMBER(txid)
			KV_MEMBER(txkey)
			KV_MEMBER(address)
		}
	};

	struct response {
		uint64_t amount;
		std::vector<TransactionOutput> outputs;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(amount)
			KV_MEMBER(outputs)
			KV_MEMBER(status)
		}
	};
};

//-----------------------------------------------
struct K_COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY {
	struct request {
		std::string txid;
		std::string view_key;
		std::string address;

		void serialize(ISerializer &s) {
			KV_MEMBER(txid)
			KV_MEMBER(view_key)
			KV_MEMBER(address)
		}
	};

	struct response {
		uint64_t amount;
		std::vector<TransactionOutput> outputs;
		uint32_t confirmations = 0;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(amount)
			KV_MEMBER(outputs)
			KV_MEMBER(confirmations)
			KV_MEMBER(status)
		}
	};
};

struct COMMAND_RPC_VALIDATE_ADDRESS {
  struct request {
    std::string address;

    void serialize(ISerializer &s) {
      KV_MEMBER(address)
    }
  };

  struct response {
    bool isvalid;
    std::string address;
    std::string spendPublicKey;
    std::string viewPublicKey;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(isvalid)
      KV_MEMBER(address)
      KV_MEMBER(spendPublicKey)
      KV_MEMBER(viewPublicKey)
      KV_MEMBER(status)
    }
  };
};

struct COMMAND_RPC_VERIFY_MESSAGE {
	struct request {
		std::string message;
		std::string address;
		std::string signature;

		void serialize(ISerializer &s) {
			KV_MEMBER(message)
			KV_MEMBER(address)
			KV_MEMBER(signature)
		}
	};

	struct response {
		bool sig_valid;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(sig_valid)
			KV_MEMBER(status)
		}
	};
};

struct COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS {
  struct request {
    std::vector<uint32_t> blockHeights;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHeights);
    }
  };

  struct response {
    std::vector<BlockDetails> blocks;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(blocks)
    }
  };
};

struct COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES {
  struct request {
    std::vector<Crypto::Hash> blockHashes;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHashes);
    }
  };

  struct response {
    std::vector<BlockDetails> blocks;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(blocks)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT {
  struct request {
    uint32_t blockHeight;

    void serialize(ISerializer& s) {
      KV_MEMBER(blockHeight)
    }
  };

  struct response {
    BlockDetails block;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(block)
    }
  };
};

struct COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH {
  struct request {
    std::string hash;

    void serialize(ISerializer& s) {
      KV_MEMBER(hash)
    }
  };

  struct response {
    BlockDetails block;
    std::string status;

    void serialize(ISerializer& s) {
      KV_MEMBER(status)
      KV_MEMBER(block)
    }
  };
};

struct COMMAND_RPC_GET_BLOCKS_HASHES_BY_TIMESTAMPS {
  struct request {
    uint64_t timestampBegin;
    uint64_t timestampEnd;
	uint32_t limit;

    void serialize(ISerializer &s) {
      KV_MEMBER(timestampBegin)
      KV_MEMBER(timestampEnd)
      KV_MEMBER(limit)
    }
  };

  struct response {
    std::vector<Crypto::Hash> blockHashes;
	uint32_t count;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(count)
      KV_MEMBER(blockHashes)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID {
  struct request {
    Crypto::Hash paymentId;

    void serialize(ISerializer &s) {
      KV_MEMBER(paymentId)
    }
  };

  struct response {
    std::vector<Crypto::Hash> transactionHashes;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(transactionHashes);
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES {
  struct request {
    std::vector<Crypto::Hash> transactionHashes;

    void serialize(ISerializer &s) {
      KV_MEMBER(transactionHashes);
    }
  };

  struct response {
    std::vector<TransactionDetails> transactions;
    std::string status;

    void serialize(ISerializer &s) {
      KV_MEMBER(status)
      KV_MEMBER(transactions)
    }
  };
};

struct COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH {
	struct request {
		Crypto::Hash hash;

		void serialize(ISerializer &s) {
			KV_MEMBER(hash);
		}
	};

	struct response {
		TransactionDetails transaction;
		std::string status;

		void serialize(ISerializer &s) {
			KV_MEMBER(status)
			KV_MEMBER(transaction)
		}
	};
};

//-----------------------------------------------
struct reserve_proof_entry
{
	Crypto::Hash txid;
	uint64_t index_in_tx;
	Crypto::PublicKey shared_secret;
	Crypto::KeyImage key_image;
	Crypto::Signature shared_secret_sig;
	Crypto::Signature key_image_sig;

	void serialize(ISerializer& s)
	{
		KV_MEMBER(txid)
		KV_MEMBER(index_in_tx)
		KV_MEMBER(shared_secret)
		KV_MEMBER(key_image)
		KV_MEMBER(shared_secret_sig)
		KV_MEMBER(key_image_sig)
	}
};

struct reserve_proof {
	std::vector<reserve_proof_entry> proofs;
	Crypto::Signature signature;

	void serialize(ISerializer &s) {
		KV_MEMBER(proofs)
		KV_MEMBER(signature)
	}
};

struct K_COMMAND_RPC_CHECK_TX_PROOF {
    struct request {
        std::string tx_id;
        std::string dest_address;
        std::string signature;

        void serialize(ISerializer &s) {
            KV_MEMBER(tx_id)
            KV_MEMBER(dest_address)
            KV_MEMBER(signature)
        }
    };

    struct response {
        bool signature_valid;
        uint64_t received_amount;
		std::vector<TransactionOutput> outputs;
		uint32_t confirmations = 0;
        std::string status;

        void serialize(ISerializer &s) {
            KV_MEMBER(signature_valid)
            KV_MEMBER(received_amount)
            KV_MEMBER(outputs)
            KV_MEMBER(confirmations)
            KV_MEMBER(status)
        }
    };
};

struct K_COMMAND_RPC_CHECK_RESERVE_PROOF {
	struct request {
		std::string address;
		std::string message;
		std::string signature;
    uint32_t height = 0;
		
		void serialize(ISerializer &s) {
			KV_MEMBER(address)
			KV_MEMBER(message)
			KV_MEMBER(signature)
      KV_MEMBER(height)
		}
	};

	struct response	{
		bool good;
		uint64_t total;
		uint64_t spent;
		uint64_t locked;

		void serialize(ISerializer &s) {
			KV_MEMBER(good)
			KV_MEMBER(total)
			KV_MEMBER(spent)
			KV_MEMBER(locked)
		}
	};
};

}
