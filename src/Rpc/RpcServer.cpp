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


#include "RpcServer.h"
#include "version.h"

#include <future>
#include <unordered_map>

// CryptoNote
#include "BlockchainExplorerData.h"
#include "Common/StringTools.h"
#include "Common/Base58.h"
#include "CryptoNoteCore/TransactionUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteCore/IBlock.h"
#include "CryptoNoteCore/Miner.h"
#include "CryptoNoteCore/TransactionExtra.h"
#include "CryptoNoteProtocol/ICryptoNoteProtocolQuery.h"

#include "P2p/NetNode.h"

#include "CoreRpcServerErrorCodes.h"
#include "JsonRpc.h"

#undef ERROR

using namespace Logging;
using namespace Crypto;
using namespace Common;

static const Crypto::SecretKey I = { { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

namespace CryptoNote {

namespace {

template <typename Command>
RpcServer::HandlerFunction binMethod(bool (RpcServer::*handler)(typename Command::request const&, typename Command::response&)) {
  return [handler](RpcServer* obj, const HttpRequest& request, HttpResponse& response) {

    boost::value_initialized<typename Command::request> req;
    boost::value_initialized<typename Command::response> res;

    if (!loadFromBinaryKeyValue(static_cast<typename Command::request&>(req), request.getBody())) {
      return false;
    }

    bool result = (obj->*handler)(req, res);
    response.setBody(storeToBinaryKeyValue(res.data()));
    return result;
  };
}

template <typename Command>
RpcServer::HandlerFunction jsonMethod(bool (RpcServer::*handler)(typename Command::request const&, typename Command::response&)) {
  return [handler](RpcServer* obj, const HttpRequest& request, HttpResponse& response) {

    boost::value_initialized<typename Command::request> req;
    boost::value_initialized<typename Command::response> res;

    if (!loadFromJson(static_cast<typename Command::request&>(req), request.getBody())) {
      return false;
    }

    bool result = (obj->*handler)(req, res);
    std::string cors_domain = obj->getCorsDomain();
    if (!cors_domain.empty()) {
      response.addHeader("Access-Control-Allow-Origin", cors_domain);
      response.addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      response.addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    }
    response.addHeader("Content-Type", "application/json");
    response.setBody(storeToJson(res.data()));
    return result;
  };
}

}

std::unordered_map<std::string, RpcServer::RpcHandler<RpcServer::HandlerFunction>> RpcServer::s_handlers = {
  
  // binary handlers
  { "/getblocks.bin", { binMethod<COMMAND_RPC_GET_BLOCKS_FAST>(&RpcServer::on_get_blocks), false } },
  { "/queryblocks.bin", { binMethod<COMMAND_RPC_QUERY_BLOCKS>(&RpcServer::on_query_blocks), false } },
  { "/queryblockslite.bin", { binMethod<COMMAND_RPC_QUERY_BLOCKS_LITE>(&RpcServer::on_query_blocks_lite), false } },
  { "/get_o_indexes.bin", { binMethod<COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES>(&RpcServer::on_get_indexes), false } },
  { "/getrandom_outs.bin", { binMethod<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS>(&RpcServer::on_get_random_outs), false } },
  { "/get_pool_changes.bin", { binMethod<COMMAND_RPC_GET_POOL_CHANGES>(&RpcServer::onGetPoolChanges), false } },
  { "/get_pool_changes_lite.bin", { binMethod<COMMAND_RPC_GET_POOL_CHANGES_LITE>(&RpcServer::onGetPoolChangesLite), false } },

  // http get json handlers
  { "/getinfo", { jsonMethod<COMMAND_RPC_GET_INFO>(&RpcServer::on_get_info), true } },
  { "/getheight", { jsonMethod<COMMAND_RPC_GET_HEIGHT>(&RpcServer::on_get_height), true } },
  { "/feeaddress", { jsonMethod<COMMAND_RPC_GET_FEE_ADDRESS>(&RpcServer::on_get_fee_address), true } },
  { "/peers", { jsonMethod<COMMAND_RPC_GET_PEER_LIST>(&RpcServer::on_get_peer_list), true } }, // deprecated
  { "/getpeers", { jsonMethod<COMMAND_RPC_GET_PEER_LIST>(&RpcServer::on_get_peer_list), true } },
  { "/paymentid", { jsonMethod<COMMAND_RPC_GEN_PAYMENT_ID>(&RpcServer::on_get_payment_id), true } },

  // rpc post json handlers
  { "/gettransactions", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS>(&RpcServer::on_get_transactions), false } },
  { "/sendrawtransaction", { jsonMethod<COMMAND_RPC_SEND_RAW_TX>(&RpcServer::on_send_raw_tx), false } },
  { "/getblocks", { jsonMethod<COMMAND_RPC_GET_BLOCKS_FAST>(&RpcServer::on_get_blocks), false } },
  { "/queryblocks", { jsonMethod<COMMAND_RPC_QUERY_BLOCKS>(&RpcServer::on_query_blocks), false } },
  { "/queryblockslite", { jsonMethod<COMMAND_RPC_QUERY_BLOCKS_LITE>(&RpcServer::on_query_blocks_lite), false } },
  { "/get_o_indexes", { jsonMethod<COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES>(&RpcServer::on_get_indexes), false } },
  { "/getrandom_outs", { jsonMethod<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS>(&RpcServer::on_get_random_outs), false } },
  { "/get_pool_changes", { jsonMethod<COMMAND_RPC_GET_POOL_CHANGES>(&RpcServer::onGetPoolChanges), false } },
  { "/get_pool_changes_lite", { jsonMethod<COMMAND_RPC_GET_POOL_CHANGES_LITE>(&RpcServer::onGetPoolChangesLite), false } },
  { "/get_block_details_by_height", { jsonMethod<COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT>(&RpcServer::onGetBlockDetailsByHeight), false } },
  { "/get_blocks_details_by_heights", { jsonMethod<COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS>(&RpcServer::onGetBlocksDetailsByHeights), false } },
  { "/get_blocks_details_by_hashes", { jsonMethod<COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES>(&RpcServer::onGetBlocksDetailsByHashes), false } },
  { "/get_blocks_hashes_by_timestamps", { jsonMethod<COMMAND_RPC_GET_BLOCKS_HASHES_BY_TIMESTAMPS>(&RpcServer::onGetBlocksHashesByTimestamps), false } },
  { "/get_transaction_details_by_hashes", { jsonMethod<COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES>(&RpcServer::onGetTransactionsDetailsByHashes), false } },
  { "/get_transaction_hashes_by_payment_id", { jsonMethod<COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID>(&RpcServer::onGetTransactionHashesByPaymentId), false } },

  // json rpc
  { "/json_rpc", { std::bind(&RpcServer::processJsonRpcRequest, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), true } }
};

RpcServer::RpcServer(System::Dispatcher& dispatcher, Logging::ILogger& log, core& c, NodeServer& p2p, ICryptoNoteProtocolQuery& protocolQuery) :
  HttpServer(dispatcher, log), logger(log, "RpcServer"), m_core(c), m_p2p(p2p), m_protocolQuery(protocolQuery), blockchainExplorerDataBuilder(c, protocolQuery) {
}

void RpcServer::processRequest(const HttpRequest& request, HttpResponse& response) {
  logger(TRACE) << "RPC request came: \n" << request << std::endl;

  auto url = request.getUrl();

  auto it = s_handlers.find(url);
  if (it == s_handlers.end()) {
    response.setStatus(HttpResponse::STATUS_404);
    return;
  }

  if (!it->second.allowBusyCore && !isCoreReady()) {
    response.setStatus(HttpResponse::STATUS_500);
    response.setBody("Core is busy");
    return;
  }
  
  it->second.handler(this, request, response);
}

bool RpcServer::processJsonRpcRequest(const HttpRequest& request, HttpResponse& response) {

  using namespace JsonRpc;

  response.addHeader("Content-Type", "application/json");
  if (!m_cors_domain.empty()) {
    response.addHeader("Access-Control-Allow-Origin", m_cors_domain);
    response.addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    response.addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  }	

  JsonRpcRequest jsonRequest;
  JsonRpcResponse jsonResponse;

  try {
    logger(TRACE) << "JSON-RPC request: " << request.getBody();
    jsonRequest.parseRequest(request.getBody());
    jsonResponse.setId(jsonRequest.getId()); // copy id

    static std::unordered_map<std::string, RpcServer::RpcHandler<JsonMemberMethod>> jsonRpcHandlers = {
	
      { "getblockcount", { makeMemberMethod(&RpcServer::on_getblockcount), true } },
      { "getblockhash", { makeMemberMethod(&RpcServer::on_getblockhash), false } },
      { "getblocktemplate", { makeMemberMethod(&RpcServer::on_getblocktemplate), false } },
      { "getblockheaderbyhash", { makeMemberMethod(&RpcServer::on_get_block_header_by_hash), false } },
      { "getblockheaderbyheight", { makeMemberMethod(&RpcServer::on_get_block_header_by_height), false } },
      { "getblockbyheight", { makeMemberMethod(&RpcServer::onGetBlockDetailsByHeight), false } },
      { "getblockbyhash", { makeMemberMethod(&RpcServer::onGetBlockDetailsByHash), false } },
      { "getblocksbyheights", { makeMemberMethod(&RpcServer::onGetBlocksDetailsByHeights), false } },
      { "getblocksbyhashes", { makeMemberMethod(&RpcServer::onGetBlocksDetailsByHashes), false } },
      { "getblockshashesbytimestamps", { makeMemberMethod(&RpcServer::onGetBlocksHashesByTimestamps), false } },
      { "getblockslist", { makeMemberMethod(&RpcServer::on_blocks_list_json), false } },
      { "getlastblockheader", { makeMemberMethod(&RpcServer::on_get_last_block_header), false } },
      { "gettransaction", { makeMemberMethod(&RpcServer::onGetTransactionDetailsByHash), false } },
      { "gettransactionspool", { makeMemberMethod(&RpcServer::on_get_transactions_pool), false } },
      { "gettransactionsbypaymentid", { makeMemberMethod(&RpcServer::on_get_transactions_by_payment_id), false } },
      { "gettransactionhashesbypaymentid", { makeMemberMethod(&RpcServer::onGetTransactionHashesByPaymentId), false } },
      { "gettransactionsbyhashes", { makeMemberMethod(&RpcServer::onGetTransactionsDetailsByHashes), false } },
      { "getcurrencyid", { makeMemberMethod(&RpcServer::on_get_currency_id), true } },
      { "checktransactionkey", { makeMemberMethod(&RpcServer::on_check_tx_key), false } },
      { "checktransactionbyviewkey", { makeMemberMethod(&RpcServer::on_check_tx_with_view_key), false } },
      { "checktransactionproof", { makeMemberMethod(&RpcServer::on_check_tx_proof), false } },
      { "checkreserveproof", { makeMemberMethod(&RpcServer::on_check_reserve_proof), false } },
      { "validateaddress", { makeMemberMethod(&RpcServer::on_validate_address), false } },
      { "verifymessage", { makeMemberMethod(&RpcServer::on_verify_message), false } },
      { "submitblock", { makeMemberMethod(&RpcServer::on_submitblock), false } },

      /* Deprecated (rename, see above new names, remove only methods) */
      { "on_getblockhash", { makeMemberMethod(&RpcServer::on_getblockhash), false } },
      { "f_blocks_list_json", { makeMemberMethod(&RpcServer::on_blocks_list_json), false } },
      { "k_transactions_by_payment_id", { makeMemberMethod(&RpcServer::on_get_transactions_by_payment_id), false } },
      { "get_transaction_hashes_by_payment_id", { makeMemberMethod(&RpcServer::onGetTransactionHashesByPaymentId), false } },
      { "get_transaction_details_by_hashes", { makeMemberMethod(&RpcServer::onGetTransactionsDetailsByHashes), false } },
      { "k_transaction_details_by_hash", { makeMemberMethod(&RpcServer::onGetTransactionDetailsByHash), false } },
      { "get_blocks_details_by_heights", { makeMemberMethod(&RpcServer::onGetBlocksDetailsByHeights), false } },
      { "get_block_details_by_height", { makeMemberMethod(&RpcServer::onGetBlockDetailsByHeight), false } },
      { "get_block_details_by_hash", { makeMemberMethod(&RpcServer::onGetBlockDetailsByHash), false } },
      { "get_blocks_details_by_hashes", { makeMemberMethod(&RpcServer::onGetBlocksDetailsByHashes), false } },
      { "get_blocks_hashes_by_timestamps", { makeMemberMethod(&RpcServer::onGetBlocksHashesByTimestamps), false } },
      { "check_tx_key", { makeMemberMethod(&RpcServer::on_check_tx_key), false } },
      { "check_tx_with_view_key", { makeMemberMethod(&RpcServer::on_check_tx_with_view_key), false } },
      { "check_tx_proof", { makeMemberMethod(&RpcServer::on_check_tx_proof), false } },
      { "check_reserve_proof", { makeMemberMethod(&RpcServer::on_check_reserve_proof), false } },

      /* Deprecated (remove handlers - use BlockchainExplorer instead of Forknote) */
      { "f_block_json", { makeMemberMethod(&RpcServer::f_on_block_json), false } },
      { "f_transaction_json", { makeMemberMethod(&RpcServer::f_on_transaction_json), false } },
      { "f_on_transactions_pool_json", { makeMemberMethod(&RpcServer::f_on_transactions_pool_json), false } },
      { "f_pool_json", { makeMemberMethod(&RpcServer::f_on_pool_json), false } },
      { "f_mempool_json", { makeMemberMethod(&RpcServer::f_on_mempool_json), false } }

    };

    auto it = jsonRpcHandlers.find(jsonRequest.getMethod());
    if (it == jsonRpcHandlers.end()) {
      throw JsonRpcError(JsonRpc::errMethodNotFound);
    }

    if (!it->second.allowBusyCore && !isCoreReady()) {
      throw JsonRpcError(CORE_RPC_ERROR_CODE_CORE_BUSY, "Core is busy");
    }

    it->second.handler(this, jsonRequest, jsonResponse);

  } catch (const JsonRpcError& err) {
    jsonResponse.setError(err);
  } catch (const std::exception& e) {
    jsonResponse.setError(JsonRpcError(JsonRpc::errInternalError, e.what()));
  }

  response.setBody(jsonResponse.getBody());
  logger(TRACE) << "JSON-RPC response: " << jsonResponse.getBody();
  return true;
}

bool RpcServer::restrictRPC(const bool is_restricted) {
  m_restricted_rpc = is_restricted;
  return true;
}

bool RpcServer::enableCors(const std::string domain) {
  m_cors_domain = domain;
  return true;
}

std::string RpcServer::getCorsDomain() {
  return m_cors_domain;
}

bool RpcServer::setFeeAddress(const std::string& fee_address, const AccountPublicAddress& fee_acc) {
  m_fee_address = fee_address;
  m_fee_acc = fee_acc;
  return true;
}

bool RpcServer::setViewKey(const std::string& view_key) {
  Crypto::Hash private_view_key_hash;
  size_t size;
  if (!Common::fromHex(view_key, &private_view_key_hash, sizeof(private_view_key_hash), size) || size != sizeof(private_view_key_hash)) {
    logger(INFO) << "Could not parse private view key";
    return false;
  }
  m_view_key = *(struct Crypto::SecretKey *) &private_view_key_hash;
  return true;
}

bool RpcServer::setContactInfo(const std::string& contact) {
  m_contact_info = contact;
  return true;
}

bool RpcServer::isCoreReady() {
  return m_core.currency().isTestnet() || m_p2p.get_payload_object().isSynchronized();
}

bool RpcServer::masternode_check_incoming_tx(const BinaryArray& tx_blob) {
	Crypto::Hash tx_hash = NULL_HASH;
	Crypto::Hash tx_prefixt_hash = NULL_HASH;
	Transaction tx;
	if (!parseAndValidateTransactionFromBinaryArray(tx_blob, tx, tx_hash, tx_prefixt_hash)) {
		logger(INFO) << "Could not parse tx from blob";
		return false;
	}
	
	// always relay fusion transactions
	uint64_t inputs_amount = 0;
	get_inputs_money_amount(tx, inputs_amount);
	uint64_t outputs_amount = get_outs_money_amount(tx);

	const uint64_t fee = inputs_amount - outputs_amount;
	if (fee == 0 && m_core.currency().isFusionTransaction(tx, tx_blob.size(), m_core.get_current_blockchain_height() - 1)) {
		logger(DEBUGGING) << "Masternode received fusion transaction, relaying with no fee check";
		return true;
	}

	CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

	std::vector<uint32_t> out;
	uint64_t amount;

	/*
	if (!CryptoNote::findOutputsToAccount(transaction, m_fee_acc, m_view_key, out, amount)) {
		logger(INFO) << "Could not find outputs to masternode fee address";
		return false;
	}
	if (amount != 0) {
		logger(INFO) << "Masternode received relayed transaction fee: " << m_core.currency().formatAmount(amount) << " DNX";
		return true;
	}
	*/

	return false;
}

//
// Binary handlers
//

bool RpcServer::on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res) {
  // TODO code duplication see InProcessNode::doGetNewBlocks()
  if (req.block_ids.empty()) {
    res.status = "Failed";
    return false;
  }

  if (req.block_ids.back() != m_core.getBlockIdByHeight(0)) {
    res.status = "Failed";
    return false;
  }

  uint32_t totalBlockCount;
  uint32_t startBlockIndex;
  std::vector<Crypto::Hash> supplement = m_core.findBlockchainSupplement(req.block_ids, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT, totalBlockCount, startBlockIndex);

  res.current_height = totalBlockCount;
  res.start_height = startBlockIndex;

  for (const auto& blockId : supplement) {
    assert(m_core.have_block(blockId));
    auto completeBlock = m_core.getBlock(blockId);
    assert(completeBlock != nullptr);

    res.blocks.resize(res.blocks.size() + 1);
    res.blocks.back().block = asString(toBinaryArray(completeBlock->getBlock()));

    res.blocks.back().txs.reserve(completeBlock->getTransactionCount());
    for (size_t i = 0; i < completeBlock->getTransactionCount(); ++i) {
      res.blocks.back().txs.push_back(asString(toBinaryArray(completeBlock->getTransaction(i))));
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_query_blocks(const COMMAND_RPC_QUERY_BLOCKS::request& req, COMMAND_RPC_QUERY_BLOCKS::response& res) {
  uint32_t startHeight;
  uint32_t currentHeight;
  uint32_t fullOffset;

  if (!m_core.queryBlocks(req.block_ids, req.timestamp, startHeight, currentHeight, fullOffset, res.items)) {
    res.status = "Failed to perform query";
    return false;
  }

  res.start_height = startHeight;
  res.current_height = currentHeight;
  res.full_offset = fullOffset;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_query_blocks_lite(const COMMAND_RPC_QUERY_BLOCKS_LITE::request& req, COMMAND_RPC_QUERY_BLOCKS_LITE::response& res) {
  uint32_t startHeight;
  uint32_t currentHeight;
  uint32_t fullOffset;
  if (!m_core.queryBlocksLite(req.blockIds, req.timestamp, startHeight, currentHeight, fullOffset, res.items)) {
    res.status = "Failed to perform query";
    return false;
  }

  res.startHeight = startHeight;
  res.currentHeight = currentHeight;
  res.fullOffset = fullOffset;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res) {
  std::vector<uint32_t> outputIndexes;
  if (!m_core.get_tx_outputs_gindexs(req.txid, outputIndexes)) {
    res.status = "Failed";
    return true;
  }

  res.o_indexes.assign(outputIndexes.begin(), outputIndexes.end());
  res.status = CORE_RPC_STATUS_OK;
  logger(TRACE) << "COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES: [" << res.o_indexes.size() << "]";
  return true;
}

bool RpcServer::on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) {
  res.status = "Failed";
  if (!m_core.get_random_outs_for_amounts(req, res)) {
    return true;
  }

  res.status = CORE_RPC_STATUS_OK;

  std::stringstream ss;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
  typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;

  std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)  {
    ss << "[" << ofa.amount << "]:";

    assert(ofa.outs.size() && "internal error: ofa.outs.size() is empty");

    std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
    {
      ss << oe.global_amount_index << " ";
    });
    ss << ENDL;
  });
  std::string s = ss.str();
  logger(TRACE) << "COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetPoolChanges(const COMMAND_RPC_GET_POOL_CHANGES::request& req, COMMAND_RPC_GET_POOL_CHANGES::response& rsp) {
  rsp.status = CORE_RPC_STATUS_OK;
  std::vector<CryptoNote::Transaction> addedTransactions;
  rsp.isTailBlockActual = m_core.getPoolChanges(req.tailBlockId, req.knownTxsIds, addedTransactions, rsp.deletedTxsIds);
  for (auto& tx : addedTransactions) {
    BinaryArray txBlob;
    if (!toBinaryArray(tx, txBlob)) {
      rsp.status = "Internal error";
      break;;
    }

    rsp.addedTxs.emplace_back(std::move(txBlob));
  }
  return true;
}


bool RpcServer::onGetPoolChangesLite(const COMMAND_RPC_GET_POOL_CHANGES_LITE::request& req, COMMAND_RPC_GET_POOL_CHANGES_LITE::response& rsp) {
  rsp.status = CORE_RPC_STATUS_OK;
  rsp.isTailBlockActual = m_core.getPoolChangesLite(req.tailBlockId, req.knownTxsIds, rsp.addedTxs, rsp.deletedTxsIds);

  return true;
}

bool RpcServer::onGetBlocksDetailsByHeights(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HEIGHTS::response& rsp) {
  std::vector<BlockDetails> blockDetails;
  for (const uint32_t& height : req.blockHeights) {
    if (m_core.get_current_blockchain_height() <= height) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
        std::string("To big height: ") + std::to_string(height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height() - 1) };
    }
    Hash block_hash = m_core.getBlockIdByHeight(height);
    Block blk;
    if (!m_core.getBlockByHash(block_hash, blk)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get block by height " + std::to_string(height) + '.' };
    }
    BlockDetails detail;
    if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, detail)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
    }
    blockDetails.push_back(detail);
  }
  rsp.blocks = std::move(blockDetails);

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetBlocksDetailsByHashes(const COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS_BY_HASHES::response& rsp) {
  std::vector<BlockDetails> blockDetails;
  for (const Crypto::Hash& hash : req.blockHashes) {
    Block blk;
    if (!m_core.getBlockByHash(hash, blk)) {
      //throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get block by hash " + Common::PodToHex(hash) + '.' };
    }
    BlockDetails detail;
    if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, detail)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
    }
    blockDetails.push_back(detail);
  }
  rsp.blocks = std::move(blockDetails);

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetBlockDetailsByHeight(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HEIGHT::response& rsp) {
  BlockDetails blockDetails;
  if (m_core.get_current_blockchain_height() <= req.blockHeight) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.blockHeight) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height() - 1) };
  }
  Hash block_hash = m_core.getBlockIdByHeight(req.blockHeight);
  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by height " + std::to_string(req.blockHeight) + '.' };
}
  if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, blockDetails)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
  }
  rsp.block = blockDetails;

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetBlockDetailsByHash(const COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_DETAILS_BY_HASH::response& rsp) {
  BlockDetails blockDetails;
  Hash block_hash;
  if (!parse_hash256(req.hash, block_hash)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "Failed to parse hex representation of block hash. Hex = " + req.hash + '.' };
  }
  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by hash. Hash = " + req.hash + '.' };
  }
  if (!blockchainExplorerDataBuilder.fillBlockDetails(blk, blockDetails)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't fill block details." };
  }
  rsp.block = blockDetails;

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetBlocksHashesByTimestamps(const COMMAND_RPC_GET_BLOCKS_HASHES_BY_TIMESTAMPS::request& req, COMMAND_RPC_GET_BLOCKS_HASHES_BY_TIMESTAMPS::response& rsp) {
  uint32_t count;
  std::vector<Crypto::Hash> blockHashes;
  if (!m_core.get_blockchain_storage().getBlockIdsByTimestamp(req.timestampBegin, req.timestampEnd, req.limit, blockHashes, count)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get blocks within timestamps " + std::to_string(req.timestampBegin) + " - " + std::to_string(req.timestampEnd) + "." };
  }
  rsp.blockHashes = std::move(blockHashes);
  rsp.count = count;
  
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetTransactionsDetailsByHashes(const COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::request& req, COMMAND_RPC_GET_TRANSACTIONS_DETAILS_BY_HASHES::response& rsp) {
  std::vector<TransactionDetails> transactionsDetails;
  transactionsDetails.reserve(req.transactionHashes.size());

  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;
  m_core.getTransactions(req.transactionHashes, txs, missed_txs, true);

  if (!txs.empty()) {
    for (const Transaction& tx : txs) {
      TransactionDetails txDetails;
      if (!blockchainExplorerDataBuilder.fillTransactionDetails(tx, txDetails)) {
        throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
          "Internal error: can't fill transaction details." };
      }
      transactionsDetails.push_back(txDetails);
    }

    rsp.transactions = std::move(transactionsDetails);
    rsp.status = CORE_RPC_STATUS_OK;
  }
  if (txs.empty() || !missed_txs.empty()) {
    std::ostringstream ss;
    std::string separator;
    for (auto h : missed_txs) {
      ss << separator << Common::podToHex(h);
      separator = ",";
    }
    rsp.status = "transaction(s) not found: " + ss.str() + ".";
  }

  return true;
}

bool RpcServer::onGetTransactionDetailsByHash(const COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::request& req, COMMAND_RPC_GET_TRANSACTION_DETAILS_BY_HASH::response& rsp) {
  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;
  std::vector<Crypto::Hash> hashes;
  hashes.push_back(req.hash);
  m_core.getTransactions(hashes, txs, missed_txs, true);

  if (txs.empty() || !missed_txs.empty()) {
    std::string hash_str = Common::podToHex(missed_txs.back());
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "transaction wasn't found. Hash = " + hash_str + '.' };
  }

  TransactionDetails transactionsDetails;
  if (!blockchainExplorerDataBuilder.fillTransactionDetails(txs.back(), transactionsDetails)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't fill transaction details." };
  }

  rsp.transaction = std::move(transactionsDetails);

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::onGetTransactionHashesByPaymentId(const COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::request& req, COMMAND_RPC_GET_TRANSACTION_HASHES_BY_PAYMENT_ID::response& rsp) {
  rsp.transactionHashes = m_core.getTransactionHashesByPaymentId(req.paymentId);

  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

//
// JSON handlers
//

bool RpcServer::on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res) {
  res.height = m_core.get_current_blockchain_height();
  res.difficulty = m_core.getNextBlockDifficulty();
  res.tx_count = m_core.get_blockchain_total_transactions() - res.height; //without coinbase
  res.tx_pool_size = m_core.get_pool_transactions_count();
  res.alt_blocks_count = m_core.get_alternative_blocks_count();
  uint64_t total_conn = m_p2p.get_connections_count();
  res.outgoing_connections_count = m_p2p.get_outgoing_connections_count();
  res.incoming_connections_count = total_conn - res.outgoing_connections_count;
  res.rpc_connections_count = get_connections_count();
  res.white_peerlist_size = m_p2p.getPeerlistManager().get_white_peers_count();
  res.grey_peerlist_size = m_p2p.getPeerlistManager().get_gray_peers_count();
  res.last_known_block_index = std::max(static_cast<uint32_t>(1), m_protocolQuery.getObservedHeight()) - 1;
  Crypto::Hash last_block_hash = m_core.getBlockIdByHeight(res.height - 1);
  res.top_block_hash = Common::podToHex(last_block_hash);
  res.version = CN_PROJECT_VERSION_LONG;
  res.version_num = CN_PROJECT_VERSION;
  res.version_build = CN_PROJECT_VERSION_BUILD_NO;
  res.version_remark = CN_VER_REMARK;
  res.fee_address = m_fee_address.empty() ? std::string() : m_fee_address;
  res.contact = m_contact_info.empty() ? std::string() : m_contact_info;
  res.min_tx_fee = m_core.getMinimalFee();
  res.readable_tx_fee = m_core.currency().formatAmount(m_core.getMinimalFee());
  res.start_time = (uint64_t)m_core.getStartTime();

  uint64_t alreadyGeneratedCoins = m_core.getTotalGeneratedAmount();
  // that large uint64_t number is unsafe in JavaScript environment and therefore as a JSON value so we display it as a formatted string
  res.already_generated_coins = m_core.currency().formatAmount(alreadyGeneratedCoins);
  res.block_major_version = m_core.getCurrentBlockMajorVersion();
  std::vector<size_t> blocksSizes;
  if (!m_core.getBackwardBlocksSizes(res.height - 1, blocksSizes, parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW)) {
    return false;
  }
  uint64_t sizeMedian = Common::medianValue(blocksSizes);
  uint64_t nextReward = 0;
  int64_t emissionChange = 0;
  if (!m_core.getBlockReward(res.height, res.block_major_version, sizeMedian, 0, alreadyGeneratedCoins, 0, nextReward, emissionChange)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get already generated coins for prev. block." };
  }
  res.next_reward = nextReward;
  
  if (!m_core.getBlockCumulativeDifficulty(res.height - 1, res.cumulative_difficulty)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get last cumulative difficulty." };
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res) {
  res.height = m_core.get_current_blockchain_height();
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res) {
  std::vector<Hash> vh;
  for (const auto& tx_hex_str : req.txs_hashes) {
    BinaryArray b;
    if (!fromHex(tx_hex_str, b))
    {
      res.status = "Failed to parse hex representation of transaction hash";
      return true;
    }
    if (b.size() != sizeof(Hash))
    {
      res.status = "Failed, size of data mismatch";
    }
    vh.push_back(*reinterpret_cast<const Hash*>(b.data()));
  }
  std::list<Hash> missed_txs;
  std::list<Transaction> txs;
  m_core.getTransactions(vh, txs, missed_txs);

  for (auto& tx : txs) {
    res.txs_as_hex.push_back(toHex(toBinaryArray(tx)));
  }

  for (const auto& miss_tx : missed_txs) {
    res.missed_tx.push_back(Common::podToHex(miss_tx));
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res) {
  BinaryArray tx_blob;
  if (!fromHex(req.tx_as_hex, tx_blob))
  {
    logger(INFO) << "[on_send_raw_tx]: Failed to parse tx from hexbuff: " << req.tx_as_hex;
    res.status = "Failed";
    return true;
  }

  Crypto::Hash transactionHash = Crypto::cn_fast_hash(tx_blob.data(), tx_blob.size());
  logger(DEBUGGING) << "transaction " << transactionHash << " came in on_send_raw_tx";

  tx_verification_context tvc = boost::value_initialized<tx_verification_context>();
  if (!m_core.handle_incoming_tx(tx_blob, tvc, false))
  {
    logger(INFO) << "[on_send_raw_tx]: Failed to process tx";
    res.status = "Failed";
    return true;
  }

  if (tvc.m_verification_failed)
  {
    logger(INFO) << "[on_send_raw_tx]: tx verification failed";
    res.status = "Failed";
    return true;
  }

  if (!tvc.m_should_be_relayed)
  {
    logger(INFO) << "[on_send_raw_tx]: tx accepted, but not relayed";
    res.status = "Not relayed";
    return true;
  }

  if (!m_fee_address.empty() && m_view_key != NULL_SECRET_KEY) {
	if (!masternode_check_incoming_tx(tx_blob)) {
	  logger(INFO) << "Transaction not relayed due to lack of masternode fee";		
      res.status = "Not relayed due to lack of node fee";
      return true;
	}
  }

  NOTIFY_NEW_TRANSACTIONS::request r;
  r.txs.push_back(asString(tx_blob));
  m_core.get_protocol()->relay_transactions(r);
  //TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_stop_daemon(const COMMAND_RPC_STOP_DAEMON::request& req, COMMAND_RPC_STOP_DAEMON::response& res) {
  if (m_restricted_rpc) {
	res.status = "Failed, restricted handle";
	return false;
  }
  if (m_core.currency().isTestnet()) {
    m_p2p.sendStopSignal();
    res.status = CORE_RPC_STATUS_OK;
  } else {
    res.status = CORE_RPC_ERROR_CODE_INTERNAL_ERROR;
    return false;
  }
  return true;
}

bool RpcServer::on_get_fee_address(const COMMAND_RPC_GET_FEE_ADDRESS::request& req, COMMAND_RPC_GET_FEE_ADDRESS::response& res) {
  if (m_fee_address.empty()) {
	res.status = CORE_RPC_STATUS_OK;
	return false; 
  }
  res.fee_address = m_fee_address;
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_peer_list(const COMMAND_RPC_GET_PEER_LIST::request& req, COMMAND_RPC_GET_PEER_LIST::response& res) {
	std::list<PeerlistEntry> pl_wite;
	std::list<PeerlistEntry> pl_gray;
	m_p2p.getPeerlistManager().get_peerlist_full(pl_gray, pl_wite);
	for (const auto& pe : pl_wite) {
		std::stringstream ss;
		ss << pe.adr;
		res.peers.push_back(ss.str());
	}
	res.status = CORE_RPC_STATUS_OK;
	return true;
}

bool RpcServer::on_get_payment_id(const COMMAND_RPC_GEN_PAYMENT_ID::request& req, COMMAND_RPC_GEN_PAYMENT_ID::response& res) {
  res.payment_id = Common::podToHex(Crypto::rand<Crypto::Hash>());
  return true;
}

//------------------------------------------------------------------------------------------------------------------------------
// JSON RPC methods
//------------------------------------------------------------------------------------------------------------------------------

bool RpcServer::on_blocks_list_json(const COMMAND_RPC_GET_BLOCKS_LIST::request& req, COMMAND_RPC_GET_BLOCKS_LIST::response& res) {
  if (m_core.get_current_blockchain_height() <= req.height) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height()) };
  }

  uint32_t print_blocks_count = 10;
  if(req.count <= 1000)
    print_blocks_count = req.count;
  
  uint32_t last_height = req.height - print_blocks_count;
  if (req.height <= print_blocks_count)  {
    last_height = 0;
  }

  for (uint32_t i = req.height; i >= last_height; i--) {
    Hash block_hash = m_core.getBlockIdByHeight(i);
    Block blk;
    if (!m_core.getBlockByHash(block_hash, blk)) {
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
        "Internal error: can't get block by height. Height = " + std::to_string(i) + '.' };
    }

    size_t tx_cumulative_block_size;
    m_core.getBlockSize(block_hash, tx_cumulative_block_size);
    size_t blokBlobSize = getObjectBinarySize(blk);
    size_t minerTxBlobSize = getObjectBinarySize(blk.baseTransaction);
    difficulty_type blockDiff;
    m_core.getBlockDifficulty(static_cast<uint32_t>(i), blockDiff);

    block_short_response block_short;
    block_short.timestamp = blk.timestamp;
    block_short.height = i;
    block_short.hash = Common::podToHex(block_hash);
    block_short.cumul_size = blokBlobSize + tx_cumulative_block_size - minerTxBlobSize;
    block_short.tx_count = blk.transactionHashes.size() + 1;
    block_short.difficulty = blockDiff;
    block_short.min_tx_fee = m_core.getMinimalFeeForHeight(i);

    res.blocks.push_back(block_short);

    if (i == 0)
      break;
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::f_on_block_json(const F_COMMAND_RPC_GET_BLOCK_DETAILS::request& req, F_COMMAND_RPC_GET_BLOCK_DETAILS::response& res) {
  Hash hash;

  try {
    uint32_t height = boost::lexical_cast<uint32_t>(req.hash);
    hash = m_core.getBlockIdByHeight(height);
  } catch (boost::bad_lexical_cast &) {
    if (!parse_hash256(req.hash, hash)) {
      throw JsonRpc::JsonRpcError{
        CORE_RPC_ERROR_CODE_WRONG_PARAM,
        "Failed to parse hex representation of block hash. Hex = " + req.hash + '.' };
    }
  }

  Block blk;
  if (!m_core.getBlockByHash(hash, blk)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by hash. Hash = " + req.hash + '.' };
  }

  if (blk.baseTransaction.inputs.front().type() != typeid(BaseInput)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: coinbase transaction in the block has the wrong type" };
  }

  block_header_response block_header;
  res.block.height = boost::get<BaseInput>(blk.baseTransaction.inputs.front()).blockIndex;

  Crypto::Hash tmp_hash = m_core.getBlockIdByHeight(res.block.height);
  bool is_orphaned = hash != tmp_hash;
  fill_block_header_response(blk, is_orphaned, res.block.height, hash, block_header);

  res.block.major_version = block_header.major_version;
  res.block.minor_version = block_header.minor_version;
  res.block.timestamp = block_header.timestamp;
  res.block.prev_hash = block_header.prev_hash;
  res.block.nonce = block_header.nonce;
  res.block.hash = block_header.hash;
  res.block.depth = block_header.depth;
  res.block.orphan_status = block_header.orphan_status;
  m_core.getBlockDifficulty(res.block.height, res.block.difficulty);
  m_core.getBlockCumulativeDifficulty(res.block.height, res.block.cumulativeDifficulty);

  res.block.reward = block_header.reward;

  std::vector<size_t> blocksSizes;
  if (!m_core.getBackwardBlocksSizes(res.block.height, blocksSizes, parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW)) {
    return false;
  }
  res.block.sizeMedian = Common::medianValue(blocksSizes);

  size_t blockSize = 0;
  if (!m_core.getBlockSize(hash, blockSize)) {
    return false;
  }
  res.block.transactionsCumulativeSize = blockSize;

  size_t blokBlobSize = getObjectBinarySize(blk);
  size_t minerTxBlobSize = getObjectBinarySize(blk.baseTransaction);
  res.block.blockSize = blokBlobSize + res.block.transactionsCumulativeSize - minerTxBlobSize;

  uint64_t alreadyGeneratedCoins;
  if (!m_core.getAlreadyGeneratedCoins(hash, alreadyGeneratedCoins)) {
    return false;
  }
  res.block.alreadyGeneratedCoins = std::to_string(alreadyGeneratedCoins);

  if (!m_core.getGeneratedTransactionsNumber(res.block.height, res.block.alreadyGeneratedTransactions)) {
    return false;
  }

  uint64_t prevBlockGeneratedCoins = 0;
  if (res.block.height > 0) {
    if (!m_core.getAlreadyGeneratedCoins(blk.previousBlockHash, prevBlockGeneratedCoins)) {
      return false;
    }
  }
  uint64_t maxReward = 0;
  uint64_t currentReward = 0;
  int64_t emissionChange = 0;
  size_t blockGrantedFullRewardZone =  CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
  res.block.effectiveSizeMedian = std::max(res.block.sizeMedian, blockGrantedFullRewardZone);

  if (!m_core.getBlockReward(res.block.height, res.block.major_version, res.block.sizeMedian, 0, prevBlockGeneratedCoins, 0, maxReward, emissionChange)) {
    return false;
  }
  if (!m_core.getBlockReward(res.block.height, res.block.major_version, res.block.sizeMedian, res.block.transactionsCumulativeSize, prevBlockGeneratedCoins, 0, currentReward, emissionChange)) {
    return false;
  }

  res.block.baseReward = maxReward;
  if (maxReward == 0 && currentReward == 0) {
    res.block.penalty = static_cast<double>(0);
  } else {
    if (maxReward < currentReward) {
      return false;
    }
    res.block.penalty = static_cast<double>(maxReward - currentReward) / static_cast<double>(maxReward);
  }

  // Base transaction adding
  f_transaction_short_response transaction_short;
  transaction_short.hash = Common::podToHex(getObjectHash(blk.baseTransaction));
  transaction_short.fee = 0;
  transaction_short.amount_out = get_outs_money_amount(blk.baseTransaction);
  transaction_short.size = getObjectBinarySize(blk.baseTransaction);
  res.block.transactions.push_back(transaction_short);


  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;
  m_core.getTransactions(blk.transactionHashes, txs, missed_txs);

  res.block.totalFeeAmount = 0;

  for (const Transaction& tx : txs) {
    f_transaction_short_response transaction_short;
    uint64_t amount_in = 0;
    get_inputs_money_amount(tx, amount_in);
    uint64_t amount_out = get_outs_money_amount(tx);

    transaction_short.hash = Common::podToHex(getObjectHash(tx));
    transaction_short.fee = amount_in - amount_out;
    transaction_short.amount_out = amount_out;
    transaction_short.size = getObjectBinarySize(tx);
    res.block.transactions.push_back(transaction_short);

    res.block.totalFeeAmount += transaction_short.fee;
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::f_on_transaction_json(const F_COMMAND_RPC_GET_TRANSACTION_DETAILS::request& req, F_COMMAND_RPC_GET_TRANSACTION_DETAILS::response& res) {
  Hash hash;

  if (!parse_hash256(req.hash, hash)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "Failed to parse hex representation of transaction hash. Hex = " + req.hash + '.' };
  }

  std::vector<Crypto::Hash> tx_ids;
  tx_ids.push_back(hash);

  std::list<Crypto::Hash> missed_txs;
  std::list<Transaction> txs;
  m_core.getTransactions(tx_ids, txs, missed_txs, true);

  if (1 == txs.size()) {
    res.tx = txs.front();
  } else {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "transaction wasn't found. Hash = " + req.hash + '.' };
  }

  Crypto::Hash blockHash;
  uint32_t blockHeight;
  if (m_core.getBlockContainingTx(hash, blockHash, blockHeight)) {
    Block blk;
    if (m_core.getBlockByHash(blockHash, blk)) {
      size_t tx_cumulative_block_size;
      m_core.getBlockSize(blockHash, tx_cumulative_block_size);
      size_t blokBlobSize = getObjectBinarySize(blk);
      size_t minerTxBlobSize = getObjectBinarySize(blk.baseTransaction);
      block_short_response block_short;

      block_short.timestamp = blk.timestamp;
      block_short.height = blockHeight;
      block_short.hash = Common::podToHex(blockHash);
      block_short.cumul_size = blokBlobSize + tx_cumulative_block_size - minerTxBlobSize;
      block_short.tx_count = blk.transactionHashes.size() + 1;
      res.block = block_short;
      res.txDetails.confirmations = m_protocolQuery.getObservedHeight() - blockHeight;
    }
  }

  uint64_t amount_in = 0;
  get_inputs_money_amount(res.tx, amount_in);
  uint64_t amount_out = get_outs_money_amount(res.tx);

  res.txDetails.hash = Common::podToHex(getObjectHash(res.tx));
  res.txDetails.fee = amount_in - amount_out;
  if (amount_in == 0)
    res.txDetails.fee = 0;
  res.txDetails.amount_out = amount_out;
  res.txDetails.size = getObjectBinarySize(res.tx);

  uint64_t mixin;
  if (!m_core.getMixin(res.tx, mixin)) {
    return false;
  }
  res.txDetails.mixin = mixin;

  Crypto::Hash paymentId;
  if (CryptoNote::getPaymentIdFromTxExtra(res.tx.extra, paymentId)) {
    res.txDetails.paymentId = Common::podToHex(paymentId);
  } else {
    res.txDetails.paymentId = "";
  }

  res.txDetails.extra.raw = res.tx.extra;

  std::vector<CryptoNote::TransactionExtraField> txExtraFields;
  parseTransactionExtra(res.tx.extra, txExtraFields);
  for (const CryptoNote::TransactionExtraField& field : txExtraFields) {
    if (typeid(CryptoNote::TransactionExtraPadding) == field.type()) {
      res.txDetails.extra.padding.push_back(std::move(boost::get<CryptoNote::TransactionExtraPadding>(field).size));
    }
    else if (typeid(CryptoNote::TransactionExtraPublicKey) == field.type()) {
      res.txDetails.extra.publicKey = CryptoNote::getTransactionPublicKeyFromExtra(res.tx.extra);
    }
    else if (typeid(CryptoNote::TransactionExtraNonce) == field.type()) {
      res.txDetails.extra.nonce.push_back(Common::toHex(boost::get<CryptoNote::TransactionExtraNonce>(field).nonce.data(), boost::get<CryptoNote::TransactionExtraNonce>(field).nonce.size()));
    }
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

/* Deprecated */
bool RpcServer::f_on_pool_json(const F_COMMAND_RPC_GET_POOL::request& req, F_COMMAND_RPC_GET_POOL::response& res) {
  auto pool = m_core.getPoolTransactions();
  for (const Transaction &tx : pool) {
    f_transaction_short_response transaction_short;

    uint64_t amount_in = getInputAmount(tx);
    uint64_t amount_out = getOutputAmount(tx);

    transaction_short.hash = Common::podToHex(getObjectHash(tx));
    transaction_short.fee = amount_in < amount_out + parameters::MINIMUM_FEE ? parameters::MINIMUM_FEE : amount_in - amount_out;
    transaction_short.amount_out = amount_out;
    transaction_short.size = getObjectBinarySize(tx);
    res.transactions.push_back(transaction_short);
  }
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

/* Deprecated */
bool RpcServer::f_on_mempool_json(const COMMAND_RPC_GET_MEMPOOL::request& req, COMMAND_RPC_GET_MEMPOOL::response& res) {
  auto pool = m_core.getMemoryPool();
  for (const CryptoNote::tx_memory_pool::TransactionDetails &txd : pool) {
    f_mempool_transaction_response mempool_transaction;
    uint64_t amount_out = getOutputAmount(txd.tx);

    mempool_transaction.hash = Common::podToHex(txd.id);
    mempool_transaction.fee = txd.fee;
    mempool_transaction.amount_out = amount_out;
    mempool_transaction.size = txd.blobSize;
    mempool_transaction.receiveTime = txd.receiveTime;
    mempool_transaction.keptByBlock = txd.keptByBlock;
    mempool_transaction.max_used_block_height = txd.maxUsedBlock.height;
    mempool_transaction.max_used_block_id = Common::podToHex(txd.maxUsedBlock.id);
    mempool_transaction.last_failed_height = txd.lastFailedBlock.height;
    mempool_transaction.last_failed_id = Common::podToHex(txd.lastFailedBlock.id);
    res.mempool.push_back(mempool_transaction);
  }
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions_pool(const COMMAND_RPC_GET_TRANSACTIONS_POOL::request& req, COMMAND_RPC_GET_TRANSACTIONS_POOL::response& res) {
  auto pool = m_core.getMemoryPool();
  for (const CryptoNote::tx_memory_pool::TransactionDetails &txd : pool) {
    transaction_pool_response mempool_transaction;
    mempool_transaction.hash = Common::podToHex(txd.id);
    mempool_transaction.fee = txd.fee;
    mempool_transaction.amount_out = getOutputAmount(txd.tx);
    mempool_transaction.size = txd.blobSize;
    mempool_transaction.receiveTime = txd.receiveTime;
    res.transactions.push_back(mempool_transaction);
  }
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_transactions_by_payment_id(const COMMAND_RPC_GET_TRANSACTIONS_BY_PAYMENT_ID::request& req, COMMAND_RPC_GET_TRANSACTIONS_BY_PAYMENT_ID::response& res) {
	if (!req.payment_id.size()) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Wrong parameters, expected payment_id" };
	}
	logger(Logging::DEBUGGING, Logging::WHITE) << "RPC request came: Search by Payment ID: " << req.payment_id;

	Crypto::Hash paymentId;
	std::vector<Transaction> transactions;

	if (!parse_hash256(req.payment_id, paymentId)) {
		throw JsonRpc::JsonRpcError{
			CORE_RPC_ERROR_CODE_WRONG_PARAM,
			"Failed to parse Payment ID: " + req.payment_id + '.' };
	}

	if (!m_core.getTransactionsByPaymentId(paymentId, transactions)) {
		throw JsonRpc::JsonRpcError{
			CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
			"Internal error: can't get transactions by Payment ID: " + req.payment_id + '.' };
	}

	for (const Transaction& tx : transactions) {
		f_transaction_short_response transaction_short;
		uint64_t amount_in = 0;
		get_inputs_money_amount(tx, amount_in);
		uint64_t amount_out = get_outs_money_amount(tx);

		transaction_short.hash = Common::podToHex(getObjectHash(tx));
		transaction_short.fee = amount_in - amount_out;
		transaction_short.amount_out = amount_out;
		transaction_short.size = getObjectBinarySize(tx);
		res.transactions.push_back(transaction_short);
	}

	res.status = CORE_RPC_STATUS_OK;
	return true;
}

/* Deprecated */
bool RpcServer::f_on_transactions_pool_json(const F_COMMAND_RPC_GET_POOL::request& req, F_COMMAND_RPC_GET_POOL::response& res) {
  auto pool = m_core.getPoolTransactions();
  for (const Transaction &tx : pool) {
    f_transaction_short_response transaction_short;
    uint64_t amount_in = getInputAmount(tx);
    uint64_t amount_out = getOutputAmount(tx);

    transaction_short.hash = Common::podToHex(getObjectHash(tx));
    transaction_short.fee = amount_in - amount_out;
    transaction_short.amount_out = amount_out;
    transaction_short.size = getObjectBinarySize(tx);
    res.transactions.push_back(transaction_short);
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res) {
  res.count = m_core.get_current_blockchain_height();
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res) {
  if (req.size() != 1) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Wrong parameters, expected height" };
  }

  uint32_t h = static_cast<uint32_t>(req[0]);
  Crypto::Hash blockId = m_core.getBlockIdByHeight(h);
  if (blockId == NULL_HASH) {
    throw JsonRpc::JsonRpcError{ 
      CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(h) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height())
    };
  }

  res = Common::podToHex(blockId);
  return true;
}

namespace {
  uint64_t slow_memmem(void* start_buff, size_t buflen, void* pat, size_t patlen)
  {
    void* buf = start_buff;
    void* end = (char*)buf + buflen - patlen;
    while ((buf = memchr(buf, ((char*)pat)[0], buflen)))
    {
      if (buf>end)
        return 0;
      if (memcmp(buf, pat, patlen) == 0)
        return (char*)buf - (char*)start_buff;
      buf = (char*)buf + 1;
    }
    return 0;
  }
}

bool RpcServer::on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res) {
  if (req.reserve_size > TX_EXTRA_NONCE_MAX_COUNT) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_RESERVE_SIZE, "To big reserved size, maximum 255" };
  }

  AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();

  if (!req.wallet_address.size() || !m_core.currency().parseAccountAddressString(req.wallet_address, acc)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_WALLET_ADDRESS, "Failed to parse wallet address" };
  }

  Block b = boost::value_initialized<Block>();
  CryptoNote::BinaryArray blob_reserve;
  blob_reserve.resize(req.reserve_size, 0);
  if (!m_core.get_block_template(b, acc, res.difficulty, res.height, blob_reserve)) {
    logger(ERROR) << "Failed to create block template";
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
  }

  BinaryArray block_blob = toBinaryArray(b);
  PublicKey tx_pub_key = CryptoNote::getTransactionPublicKeyFromExtra(b.baseTransaction.extra);
  if (tx_pub_key == NULL_PUBLIC_KEY) {
    logger(ERROR) << "Failed to find tx pub key in coinbase extra";
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to find tx pub key in coinbase extra" };
  }

  if (0 < req.reserve_size) {
    res.reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
    if (!res.reserved_offset) {
      logger(ERROR) << "Failed to find tx pub key in blockblob";
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
    }
    res.reserved_offset += sizeof(tx_pub_key) + 3; //3 bytes: tag for TX_EXTRA_TAG_PUBKEY(1 byte), tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
    if (res.reserved_offset + req.reserve_size > block_blob.size()) {
      logger(ERROR) << "Failed to calculate offset for reserved bytes";
      throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to create block template" };
    }
  } else {
    res.reserved_offset = 0;
  }

  BinaryArray hashing_blob;
  if (!get_block_hashing_blob(b, hashing_blob)) {
	  logger(ERROR) << "Failed to get blockhashing_blob";
	  throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: failed to get blockhashing_blob" };
  }

  res.blocktemplate_blob = toHex(block_blob);
  res.blockhashing_blob = toHex(hashing_blob);
  res.status = CORE_RPC_STATUS_OK;

  return true;
}

bool RpcServer::on_get_currency_id(const COMMAND_RPC_GET_CURRENCY_ID::request& /*req*/, COMMAND_RPC_GET_CURRENCY_ID::response& res) {
  Hash currencyId = m_core.currency().genesisBlockHash();
  res.currency_id_blob = Common::podToHex(currencyId);
  return true;
}

bool RpcServer::on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res) {

  if (req.size() != 1) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Wrong param" };
  }

  BinaryArray blockblob;
  if (!fromHex(req[0], blockblob)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_BLOCKBLOB, "Wrong block blob" };
  }

  block_verification_context bvc = boost::value_initialized<block_verification_context>();

  m_core.handle_incoming_block_blob(blockblob, bvc, true, true);

  if (!bvc.m_added_to_main_chain) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_BLOCK_NOT_ACCEPTED, "Block not accepted" };
  }

  res.status = CORE_RPC_STATUS_OK;
  return true;
}


namespace {
  uint64_t get_block_reward(const Block& blk) {
    uint64_t reward = 0;
    for (const TransactionOutput& out : blk.baseTransaction.outputs) {
      reward += out.amount;
    }
    return reward;
  }
}

void RpcServer::fill_block_header_response(const Block& blk, bool orphan_status, uint32_t height, const Hash& hash, block_header_response& responce) {
  responce.major_version = blk.majorVersion;
  responce.minor_version = blk.minorVersion;
  responce.timestamp = blk.timestamp;
  responce.prev_hash = Common::podToHex(blk.previousBlockHash);
  responce.nonce = blk.nonce;
  responce.orphan_status = orphan_status;
  responce.height = height;
  responce.depth = m_core.get_current_blockchain_height() - height - 1;
  responce.hash = Common::podToHex(hash);
  m_core.getBlockDifficulty(static_cast<uint32_t>(height), responce.difficulty);
  responce.reward = get_block_reward(blk);
}

bool RpcServer::on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res) {
  uint32_t last_block_height;
  Hash last_block_hash;
  
  m_core.get_blockchain_top(last_block_height, last_block_hash);

  Block last_block;
  if (!m_core.getBlockByHash(last_block_hash, last_block)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Internal error: can't get last block hash." };
  }
  Crypto::Hash tmp_hash = m_core.getBlockIdByHeight(last_block_height);
  bool is_orphaned = last_block_hash != tmp_hash;
  fill_block_header_response(last_block, is_orphaned, last_block_height, last_block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res) {
  Hash block_hash;

  if (!parse_hash256(req.hash, block_hash)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_WRONG_PARAM,
      "Failed to parse hex representation of block hash. Hex = " + req.hash + '.' };
  }

  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by hash. Hash = " + req.hash + '.' };
  }

  if (blk.baseTransaction.inputs.front().type() != typeid(BaseInput)) {
    throw JsonRpc::JsonRpcError{
      CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: coinbase transaction in the block has the wrong type" };
  }

  uint32_t block_height = boost::get<BaseInput>(blk.baseTransaction.inputs.front()).blockIndex;
  Crypto::Hash tmp_hash = m_core.getBlockIdByHeight(block_height);
  bool is_orphaned = block_hash != tmp_hash;
  fill_block_header_response(blk, is_orphaned, block_height, block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res) {
  if (m_core.get_current_blockchain_height() <= req.height) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_TOO_BIG_HEIGHT,
      std::string("To big height: ") + std::to_string(req.height) + ", current blockchain height = " + std::to_string(m_core.get_current_blockchain_height()) };
  }

  Hash block_hash = m_core.getBlockIdByHeight(static_cast<uint32_t>(req.height));
  Block blk;
  if (!m_core.getBlockByHash(block_hash, blk)) {
    throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR,
      "Internal error: can't get block by height. Height = " + std::to_string(req.height) + '.' };
  }
  
  Crypto::Hash tmp_hash = m_core.getBlockIdByHeight(req.height);
  bool is_orphaned = block_hash != tmp_hash;
  fill_block_header_response(blk, is_orphaned, req.height, block_hash, res.block_header);
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_check_tx_key(const K_COMMAND_RPC_CHECK_TX_KEY::request& req, K_COMMAND_RPC_CHECK_TX_KEY::response& res) {
	// parse txid
	Crypto::Hash txid;
	if (!parse_hash256(req.txid, txid)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
	}
	// parse address
	CryptoNote::AccountPublicAddress address;
	if (!m_core.currency().parseAccountAddressString(req.address, address)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.address + '.' };
	}
	// parse txkey
	Crypto::Hash tx_key_hash;
	size_t size;
	if (!Common::fromHex(req.txkey, &tx_key_hash, sizeof(tx_key_hash), size) || size != sizeof(tx_key_hash)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txkey" };
	}
	Crypto::SecretKey tx_key = *(struct Crypto::SecretKey *) &tx_key_hash;

	// fetch tx
	Transaction tx;
	std::vector<Crypto::Hash> tx_ids;
	tx_ids.push_back(txid);
	std::list<Crypto::Hash> missed_txs;
	std::list<Transaction> txs;
	m_core.getTransactions(tx_ids, txs, missed_txs, true);

	if (1 == txs.size()) {
		tx = txs.front();
	}
	else {
		throw JsonRpc::JsonRpcError{
			CORE_RPC_ERROR_CODE_WRONG_PARAM,
			"Couldn't find transaction with hash: " + req.txid + '.' };
	}
	CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

	// obtain key derivation
	Crypto::KeyDerivation derivation;
	if (!Crypto::generate_key_derivation(address.viewPublicKey, tx_key, derivation))
	{
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to generate key derivation from supplied parameters" };
	}
	
	// look for outputs
	uint64_t received(0);
	size_t keyIndex(0);
	std::vector<TransactionOutput> outputs;
	try {
		for (const TransactionOutput& o : transaction.outputs) {
			if (o.target.type() == typeid(KeyOutput)) {
				const KeyOutput out_key = boost::get<KeyOutput>(o.target);
				Crypto::PublicKey pubkey;
				derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
				if (pubkey == out_key.key) {
					received += o.amount;
					outputs.push_back(o);
				}
			}
			++keyIndex;
		}
	}
	catch (...)
	{
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
	}
	res.amount = received;
	res.outputs = outputs;
	res.status = CORE_RPC_STATUS_OK;
	return true;
}

bool RpcServer::on_check_tx_with_view_key(const K_COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY::request& req, K_COMMAND_RPC_CHECK_TX_WITH_PRIVATE_VIEW_KEY::response& res) {
	// parse txid
	Crypto::Hash txid;
	if (!parse_hash256(req.txid, txid)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
	}
	// parse address
	CryptoNote::AccountPublicAddress address;
	if (!m_core.currency().parseAccountAddressString(req.address, address)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.address + '.' };
	}
	// parse view key
	Crypto::Hash view_key_hash;
	size_t size;
	if (!Common::fromHex(req.view_key, &view_key_hash, sizeof(view_key_hash), size) || size != sizeof(view_key_hash)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse private view key" };
	}
	Crypto::SecretKey viewKey = *(struct Crypto::SecretKey *) &view_key_hash;

	// fetch tx
	Transaction tx;
	std::vector<Crypto::Hash> tx_ids;
	tx_ids.push_back(txid);
	std::list<Crypto::Hash> missed_txs;
	std::list<Transaction> txs;
	m_core.getTransactions(tx_ids, txs, missed_txs, true);

	if (1 == txs.size()) {
		tx = txs.front();
	}
	else {
		throw JsonRpc::JsonRpcError{
			CORE_RPC_ERROR_CODE_WRONG_PARAM,
			"Couldn't find transaction with hash: " + req.txid + '.' };
	}
	CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);
	
	// get tx pub key
	Crypto::PublicKey txPubKey = getTransactionPublicKeyFromExtra(transaction.extra);

	// obtain key derivation
	Crypto::KeyDerivation derivation;
	if (!Crypto::generate_key_derivation(txPubKey, viewKey, derivation))
	{
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to generate key derivation from supplied parameters" };
	}

	// look for outputs
	uint64_t received(0);
	size_t keyIndex(0);
	std::vector<TransactionOutput> outputs;
	try {
		for (const TransactionOutput& o : transaction.outputs) {
			if (o.target.type() == typeid(KeyOutput)) {
				const KeyOutput out_key = boost::get<KeyOutput>(o.target);
				Crypto::PublicKey pubkey;
				derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
				if (pubkey == out_key.key) {
					received += o.amount;
					outputs.push_back(o);
				}
			}
			++keyIndex;
		}
	}
	catch (...)
	{
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
	}
	res.amount = received;
	res.outputs = outputs;
	
	Crypto::Hash blockHash;
	uint32_t blockHeight;
	if (m_core.getBlockContainingTx(txid, blockHash, blockHeight)) {
		res.confirmations = m_protocolQuery.getObservedHeight() - blockHeight;
	}

	res.status = CORE_RPC_STATUS_OK;
	return true;
}

bool RpcServer::on_check_tx_proof(const K_COMMAND_RPC_CHECK_TX_PROOF::request& req, K_COMMAND_RPC_CHECK_TX_PROOF::response& res) {
	// parse txid
	Crypto::Hash txid;
	if (!parse_hash256(req.tx_id, txid)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse txid" };
	}
	// parse address
	CryptoNote::AccountPublicAddress address;
	if (!m_core.currency().parseAccountAddressString(req.dest_address, address)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.dest_address + '.' };
	}
	// parse pubkey r*A & signature
	const size_t header_len = strlen("ProofV1");
	if (req.signature.size() < header_len || req.signature.substr(0, header_len) != "ProofV1") {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Signature header check error" };
	}
	Crypto::PublicKey rA;
	Crypto::Signature sig;
	const size_t rA_len = Tools::Base58::encode(std::string((const char *)&rA, sizeof(Crypto::PublicKey))).size();
	const size_t sig_len = Tools::Base58::encode(std::string((const char *)&sig, sizeof(Crypto::Signature))).size();
	std::string rA_decoded;
	std::string sig_decoded;
	if (!Tools::Base58::decode(req.signature.substr(header_len, rA_len), rA_decoded)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Signature decoding error" };
	}
	if (!Tools::Base58::decode(req.signature.substr(header_len + rA_len, sig_len), sig_decoded)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Signature decoding error" };
	}
	if (sizeof(Crypto::PublicKey) != rA_decoded.size() || sizeof(Crypto::Signature) != sig_decoded.size()) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Signature decoding error" };
	}
	memcpy(&rA, rA_decoded.data(), sizeof(Crypto::PublicKey));
	memcpy(&sig, sig_decoded.data(), sizeof(Crypto::Signature));

	// fetch tx pubkey
	Transaction tx;

	std::vector<uint32_t> out;
	std::vector<Crypto::Hash> tx_ids;
	tx_ids.push_back(txid);
	std::list<Crypto::Hash> missed_txs;
	std::list<Transaction> txs;
	m_core.getTransactions(tx_ids, txs, missed_txs, true);

	if (1 == txs.size()) {
		tx = txs.front();
	}
	else {
		throw JsonRpc::JsonRpcError{
			CORE_RPC_ERROR_CODE_WRONG_PARAM,
			"transaction wasn't found. Hash = " + req.tx_id + '.' };
	}
	CryptoNote::TransactionPrefix transaction = *static_cast<const TransactionPrefix*>(&tx);

	Crypto::PublicKey R = getTransactionPublicKeyFromExtra(transaction.extra);
	if (R == NULL_PUBLIC_KEY)
	{
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Tx pubkey was not found" };
	}

	// check signature
	bool r = Crypto::check_tx_proof(txid, R, address.viewPublicKey, rA, sig);
	res.signature_valid = r;

	if (r) {

		// obtain key derivation by multiplying scalar 1 to the pubkey r*A included in the signature
		Crypto::KeyDerivation derivation;
		if (!Crypto::generate_key_derivation(rA, I, derivation)) {
			throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Failed to generate key derivation" };
		}

		// look for outputs
		uint64_t received(0);
		size_t keyIndex(0);
		std::vector<TransactionOutput> outputs;
		try {
			for (const TransactionOutput& o : transaction.outputs) {
				if (o.target.type() == typeid(KeyOutput)) {
					const KeyOutput out_key = boost::get<KeyOutput>(o.target);
					Crypto::PublicKey pubkey;
					derive_public_key(derivation, keyIndex, address.spendPublicKey, pubkey);
					if (pubkey == out_key.key) {
						received += o.amount;
						outputs.push_back(o);
					}
				}
				++keyIndex;
			}
		}
		catch (...)
		{
			throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
		}
		res.received_amount = received;
		res.outputs = outputs;

		Crypto::Hash blockHash;
		uint32_t blockHeight;
		if (m_core.getBlockContainingTx(txid, blockHash, blockHeight)) {
			res.confirmations = m_protocolQuery.getObservedHeight() - blockHeight;
		}
	}
	else {
		res.received_amount = 0;
	}

	res.status = CORE_RPC_STATUS_OK;
	return true;
}

bool RpcServer::on_check_reserve_proof(const K_COMMAND_RPC_CHECK_RESERVE_PROOF::request& req, K_COMMAND_RPC_CHECK_RESERVE_PROOF::response& res) {
	// parse address
	CryptoNote::AccountPublicAddress address;
	if (!m_core.currency().parseAccountAddressString(req.address, address)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_WRONG_PARAM, "Failed to parse address " + req.address + '.' };
	}
	
	// parse sugnature
	const char header[] = "ReserveProofV1";
	const size_t header_len = strlen(header);
	if (req.signature.size() < header_len || req.signature.substr(0, header_len) != header) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Signature header check error" };
	}

	std::string sig_decoded;
	if (!Tools::Base58::decode(req.signature.substr(header_len), sig_decoded)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Signature decoding error" };
	}

	BinaryArray ba;
	if (!Common::fromHex(sig_decoded, ba)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Proof decoding error" };
	}
	reserve_proof proof_decoded;
	if (!fromBinaryArray(proof_decoded, ba)) {
		throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "BinaryArray decoding error" };
	}

	std::vector<reserve_proof_entry>& proofs = proof_decoded.proofs;
	
	// compute signature prefix hash
	std::string prefix_data = req.message;
	prefix_data.append((const char*)&address, sizeof(CryptoNote::AccountPublicAddress));
	for (size_t i = 0; i < proofs.size(); ++i) {
		prefix_data.append((const char*)&proofs[i].key_image, sizeof(Crypto::PublicKey));
	}
	Crypto::Hash prefix_hash;
	Crypto::cn_fast_hash(prefix_data.data(), prefix_data.size(), prefix_hash);

	// fetch txes
	std::vector<Crypto::Hash> transactionHashes;
	for (size_t i = 0; i < proofs.size(); ++i) {
		transactionHashes.push_back(proofs[i].txid);
	}
	std::list<Hash> missed_txs;
	std::list<Transaction> txs;
	m_core.getTransactions(transactionHashes, txs, missed_txs);
	std::vector<Transaction> transactions;
	std::copy(txs.begin(), txs.end(), std::inserter(transactions, transactions.end()));

	// check spent status
	res.total = 0;
	res.spent = 0;
	res.locked = 0;
	for (size_t i = 0; i < proofs.size(); ++i) {
		const reserve_proof_entry& proof = proofs[i];

		CryptoNote::TransactionPrefix tx = *static_cast<const TransactionPrefix*>(&transactions[i]);
    
		bool unlocked = m_core.is_tx_spendtime_unlocked(tx.unlockTime, req.height);

		if (proof.index_in_tx >= tx.outputs.size()) {
			throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "index_in_tx is out of bound" };
		}

		const KeyOutput out_key = boost::get<KeyOutput>(tx.outputs[proof.index_in_tx].target);

		// get tx pub key
		Crypto::PublicKey txPubKey = getTransactionPublicKeyFromExtra(tx.extra);

		// check singature for shared secret
		if (!Crypto::check_tx_proof(prefix_hash, address.viewPublicKey, txPubKey, proof.shared_secret, proof.shared_secret_sig)) {
			//throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Failed to check singature for shared secret" };
			res.good = false;
			return true;
		}

		// check signature for key image
		const std::vector<const Crypto::PublicKey *>& pubs = { &out_key.key };
		if (!Crypto::check_ring_signature(prefix_hash, proof.key_image, &pubs[0], 1, &proof.key_image_sig)) {
			//throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Failed to check signature for key image" };
			res.good = false;
			return true;
		}

		// check if the address really received the fund
		Crypto::KeyDerivation derivation;
		if (!Crypto::generate_key_derivation(proof.shared_secret, I, derivation)) {
			throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Failed to generate key derivation" };
		}
		try {
			Crypto::PublicKey pubkey;
			derive_public_key(derivation, proof.index_in_tx, address.spendPublicKey, pubkey);
			if (pubkey == out_key.key) {
				uint64_t amount = tx.outputs[proof.index_in_tx].amount;
				res.total += amount;

        if (!unlocked) {
          res.locked += amount;
        }

        if (req.height != 0) {
          if (m_core.is_key_image_spent(proof.key_image, req.height)) {
            res.spent += amount;
          }
        } else {
          if (m_core.is_key_image_spent(proof.key_image)) {
            res.spent += amount;
          }
				}
			}
		}
		catch (...)
		{
			throw JsonRpc::JsonRpcError{ CORE_RPC_ERROR_CODE_INTERNAL_ERROR, "Unknown error" };
		}
		
	}

	// check signature for address spend keys
	Crypto::Signature sig = proof_decoded.signature;
	if (!Crypto::check_signature(prefix_hash, address.spendPublicKey, sig)) {
		res.good = false;
		return true;
	}

	res.good = true;

	return true;
}

bool RpcServer::on_validate_address(const COMMAND_RPC_VALIDATE_ADDRESS::request& req, COMMAND_RPC_VALIDATE_ADDRESS::response& res) {
  AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();
  bool r = m_core.currency().parseAccountAddressString(req.address, acc);
  res.isvalid = r;
  if (r) {
    res.address = m_core.currency().accountAddressAsString(acc);
    res.spendPublicKey = Common::podToHex(acc.spendPublicKey);
    res.viewPublicKey = Common::podToHex(acc.viewPublicKey);
  }
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool RpcServer::on_verify_message(const COMMAND_RPC_VERIFY_MESSAGE::request& req, COMMAND_RPC_VERIFY_MESSAGE::response& res) {
	Crypto::Hash hash;
	Crypto::cn_fast_hash(req.message.data(), req.message.size(), hash);

	AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();
	if (!m_core.currency().parseAccountAddressString(req.address, acc)) {
		throw JsonRpc::JsonRpcError(CORE_RPC_ERROR_CODE_WRONG_PARAM, std::string("Failed to parse address"));
	}

	const size_t header_len = strlen("SigV1");
	if (req.signature.size() < header_len || req.signature.substr(0, header_len) != "SigV1") {
		throw JsonRpc::JsonRpcError(CORE_RPC_ERROR_CODE_WRONG_PARAM, std::string("Signature header check error"));
	}
	std::string decoded;
	Crypto::Signature s;
	if (!Tools::Base58::decode(req.signature.substr(header_len), decoded) || sizeof(s) != decoded.size()) {
		throw JsonRpc::JsonRpcError(CORE_RPC_ERROR_CODE_WRONG_PARAM, std::string("Signature decoding error"));
		return false;
	}
	memcpy(&s, decoded.data(), sizeof(s));
	res.sig_valid = Crypto::check_signature(hash, acc.spendPublicKey, s);
	res.status = CORE_RPC_STATUS_OK;
	return true;
}

}
