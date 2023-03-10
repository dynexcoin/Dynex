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


#include "DaemonCommandsHandler.h"

#include <ctime>
#include "P2p/NetNode.h"
#include "CryptoNoteCore/Miner.h"
#include "CryptoNoteCore/Core.h"
#include "CryptoNoteProtocol/CryptoNoteProtocolHandler.h"
#include "Serialization/SerializationTools.h"
#include "version.h"
#include <boost/format.hpp>
#include "math.h"

namespace {
  template <typename T>
  static bool print_as_json(const T& obj) {
    std::cout << CryptoNote::storeToJson(obj) << ENDL;
    return true;
  }
}


DaemonCommandsHandler::DaemonCommandsHandler(CryptoNote::core& core, CryptoNote::NodeServer& srv, Logging::LoggerManager& log, const CryptoNote::ICryptoNoteProtocolQuery& protocol, CryptoNote::RpcServer* prpc_server) :
  m_core(core), m_srv(srv), logger(log, "daemon"), m_logManager(log), protocolQuery(protocol), m_prpc_server(prpc_server) {
  m_consoleHandler.setHandler("exit", boost::bind(&DaemonCommandsHandler::exit, this, boost::placeholders::_1), "Shutdown the daemon");
  m_consoleHandler.setHandler("help", boost::bind(&DaemonCommandsHandler::help, this, boost::placeholders::_1), "Show this help");
  m_consoleHandler.setHandler("print_pl", boost::bind(&DaemonCommandsHandler::print_pl, this, boost::placeholders::_1), "Print peer list");
  m_consoleHandler.setHandler("print_cn", boost::bind(&DaemonCommandsHandler::print_cn, this, boost::placeholders::_1), "Print connections");
  m_consoleHandler.setHandler("print_bc", boost::bind(&DaemonCommandsHandler::print_bc, this, boost::placeholders::_1), "Print blockchain info in a given blocks range, print_bc <begin_height> [<end_height>]");
  m_consoleHandler.setHandler("height", boost::bind(&DaemonCommandsHandler::print_height, this, boost::placeholders::_1), "Print blockchain height");
  //m_consoleHandler.setHandler("print_bci", boost::bind(&DaemonCommandsHandler::print_bci, this, _1));
  //m_consoleHandler.setHandler("print_bc_outs", boost::bind(&DaemonCommandsHandler::print_bc_outs, this, _1));
  m_consoleHandler.setHandler("print_block", boost::bind(&DaemonCommandsHandler::print_block, this, boost::placeholders::_1), "Print block, print_block <block_hash> | <block_height>");
  m_consoleHandler.setHandler("print_tx", boost::bind(&DaemonCommandsHandler::print_tx, this, boost::placeholders::_1), "Print transaction, print_tx <transaction_hash>");
  //m_consoleHandler.setHandler("start_mining", boost::bind(&DaemonCommandsHandler::start_mining, this, boost::placeholders::_1), "Start mining for specified address, start_mining <addr> [threads=1]");
  //m_consoleHandler.setHandler("stop_mining", boost::bind(&DaemonCommandsHandler::stop_mining, this, boost::placeholders::_1), "Stop mining");
  m_consoleHandler.setHandler("print_pool", boost::bind(&DaemonCommandsHandler::print_pool, this, boost::placeholders::_1), "Print transaction pool (long format)");
  m_consoleHandler.setHandler("print_pool_sh", boost::bind(&DaemonCommandsHandler::print_pool_sh, this, boost::placeholders::_1), "Print transaction pool (short format)");
  m_consoleHandler.setHandler("print_mp", boost::bind(&DaemonCommandsHandler::print_pool_count, this, boost::placeholders::_1), "Print number of transactions in memory pool");
  //m_consoleHandler.setHandler("show_hr", boost::bind(&DaemonCommandsHandler::show_hr, this, boost::placeholders::_1), "Start showing hash rate");
  //m_consoleHandler.setHandler("hide_hr", boost::bind(&DaemonCommandsHandler::hide_hr, this, boost::placeholders::_1), "Stop showing hash rate");
  m_consoleHandler.setHandler("set_log", boost::bind(&DaemonCommandsHandler::set_log, this, boost::placeholders::_1), "set_log <level> - Change current log level, <level> is a number 0-4");
  m_consoleHandler.setHandler("print_diff", boost::bind(&DaemonCommandsHandler::print_diff, this, boost::placeholders::_1), "Difficulty for next block");
  m_consoleHandler.setHandler("print_ban", boost::bind(&DaemonCommandsHandler::print_ban, this, boost::placeholders::_1), "Print banned nodes");
  m_consoleHandler.setHandler("ban", boost::bind(&DaemonCommandsHandler::ban, this, boost::placeholders::_1), "Ban a given <IP> for a given amount of <seconds>, ban <IP> [<seconds>]");
  m_consoleHandler.setHandler("unban", boost::bind(&DaemonCommandsHandler::unban, this, boost::placeholders::_1), "Unban a given <IP>, unban <IP>");
  m_consoleHandler.setHandler("status", boost::bind(&DaemonCommandsHandler::status, this, boost::placeholders::_1), "Show daemon status");
  // dynex chip handlers:
  //m_consoleHandler.setHandler("start_dynexchip", boost::bind(&DaemonCommandsHandler::start_dynexchip, this, boost::placeholders::_1), "Start Dynex chip, receiving DNX to specified address, start_dynexchip <addr> [threads=1]");
  //m_consoleHandler.setHandler("stop_dynexchip", boost::bind(&DaemonCommandsHandler::stop_dynexchip, this, boost::placeholders::_1), "Stop Dynex chip");
}

//--------------------------------------------------------------------------------
std::string DaemonCommandsHandler::get_commands_str()
{
  std::stringstream ss;
  ss << "Dynex Daemon v" << CN_PROJECT_VERSION_LONG << ENDL;
  ss <<
  "                                                \n"
  "################################################\n"  
  "#                                              #\n"    
  "#                DYNEX DAEMON                  #\n"
  "#                                              #\n"    
  "#     Daemon developed by Dynex Developers     #\n"
  "#        .-( https://dynexcoin.org )-.         #\n"    
  "#                                              #\n"  
  "################################################\n"    
  "                                                \n" << ENDL;
  ss << "You can use this commands: " << ENDL;
  ss << "\n" << ENDL; 
  std::string usage = m_consoleHandler.getUsage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

//--------------------------------------------------------------------------------
std::string DaemonCommandsHandler::get_mining_speed(uint64_t hr) {
  if (hr > 1e12) return (boost::format("%.2f TH/s") % (hr / 1e12)).str();
  if (hr > 1e9) return (boost::format("%.2f GH/s") % (hr / 1e9)).str();
  if (hr > 1e6) return (boost::format("%.2f MH/s") % (hr / 1e6)).str();
  if (hr > 1e3) return (boost::format("%.2f kH/s") % (hr / 1e3)).str();
  return (boost::format("%.0f H/s") % hr).str();
}

//--------------------------------------------------------------------------------
float DaemonCommandsHandler::get_sync_percentage(uint64_t height, uint64_t target_height) {
  target_height = target_height ? target_height < height ? height : target_height : height;
  float pc = 100.0f * height / target_height;
  if (height < target_height && pc > 99.9f){
    return 99.9f; // to avoid 100% when not fully synced
  }
    return pc;
}

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::exit(const std::vector<std::string>& args) {
  m_consoleHandler.requestStop();
  m_srv.sendStopSignal();
  return true;
}

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::help(const std::vector<std::string>& args) {
  std::cout << get_commands_str() << ENDL;
  return true;
}

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::status(const std::vector<std::string>& args) {
  uint32_t height = m_core.get_current_blockchain_height() - 1;
  uint64_t difficulty = m_core.getNextBlockDifficulty();
  size_t tx_pool_size = m_core.get_pool_transactions_count();
  size_t alt_blocks_count = m_core.get_alternative_blocks_count();
  Crypto::Hash last_block_hash = m_core.getBlockIdByHeight(height);  
  uint32_t last_known_block_index = std::max(static_cast<uint32_t>(1), protocolQuery.getObservedHeight()) - 1;
  std::string coins_already_generated = m_core.currency().formatAmount(m_core.getTotalGeneratedAmount());
  std::string coins_total_supply = m_core.currency().formatAmount(CryptoNote::parameters::MONEY_SUPPLY);  
  size_t total_conn = m_srv.get_connections_count();
  size_t rpc_conn = m_prpc_server->get_connections_count();
  size_t outgoing_connections_count = m_srv.get_outgoing_connections_count();
  size_t incoming_connections_count = total_conn - outgoing_connections_count;
  size_t white_peerlist_size = m_srv.getPeerlistManager().get_white_peers_count();
  size_t grey_peerlist_size = m_srv.getPeerlistManager().get_gray_peers_count();
  uint64_t hashrate = (uint64_t) round(difficulty / CryptoNote::parameters::DIFFICULTY_TARGET);
  std::time_t uptime = std::time(nullptr) - m_core.getStartTime();
  uint8_t majorVersion = m_core.getBlockMajorVersionForHeight(height);
  bool synced = ((uint32_t)height == (uint32_t)last_known_block_index);
  uint64_t alt_block_count = m_core.get_alternative_blocks_count();

  std::cout << std::endl
    <<
    "################################################\n"  
    "#                                              #\n"    
    "#              DYNEX DAEMON STATUS             #\n"
    "#                                              #\n"  
    "################################################\n"  
    << std::endl
    << (synced ? "Dynex BlockChain Synced " : "Syncing Dynex BlockChain") << " - Block: " << height << "/" << last_known_block_index 
    << " (" << get_sync_percentage(height, last_known_block_index) << "%) "
    << "on " << (m_core.currency().isTestnet() ? "testnet, " : "mainnet, ") << std::endl
    //<< "Network Hashrate: " << get_mining_speed(hashrate)  << std::endl
    << "Generated coins: " << coins_already_generated << " of " << coins_total_supply << std::endl
    << "Last block hash: " << Common::podToHex(last_block_hash) << std::endl
	<< "Next difficulty: " << difficulty  << std::endl
	<< "Alt. blocks: " << alt_block_count << std::endl
    << "Block version: " << (int)majorVersion  << std::endl
    << "Connections: Outgoing (" << outgoing_connections_count << ") / Incoming (" << incoming_connections_count << ") " << std::endl
    << "Peers: White (" << white_peerlist_size << ") / Grey (" << grey_peerlist_size << ") " << std::endl
    << "Transactions in mempool: " << tx_pool_size << std::endl
    << "Daemon uptime: " << (unsigned int)floor(uptime / 60.0 / 60.0 / 24.0) << "d " << (unsigned int)floor(fmod((uptime / 60.0 / 60.0), 24.0)) << "h "
    << (unsigned int)floor(fmod((uptime / 60.0), 60.0)) << "m " << (unsigned int)fmod(uptime, 60.0) << "s"
    << "\n\n################# END  STATUS ##################\n"      
    << "\n" << std::endl;
  
  return true;
}

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_pl(const std::vector<std::string>& args) {
  m_srv.log_peerlist();
  return true;
}
//--------------------------------------------------------------------------------
/*
bool DaemonCommandsHandler::show_hr(const std::vector<std::string>& args)
{
  if (!m_core.get_miner().is_mining())
  {
    std::cout << "Mining is not started. You need to start mining before you can see hash rate." << ENDL;
  } else
  {
    m_core.get_miner().do_print_hashrate(true);
  }
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::hide_hr(const std::vector<std::string>& args)
{
  m_core.get_miner().do_print_hashrate(false);
  return true;
}
*/
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_bc_outs(const std::vector<std::string>& args)
{
  if (args.size() != 1)
  {
    std::cout << "need file path as parameter" << ENDL;
    return true;
  }
  m_core.print_blockchain_outs(args[0]);
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_cn(const std::vector<std::string>& args)
{
  m_srv.get_payload_object().log_connections();
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_bc(const std::vector<std::string> &args) {
  if (!args.size()) {
    std::cout << "need block index parameter" << ENDL;
    return false;
  }

  uint32_t start_index = 0;
  uint32_t end_index = 0;
  uint32_t end_block_parametr = m_core.get_current_blockchain_height();
  if (!Common::fromString(args[0], start_index)) {
    std::cout << "wrong starter block index parameter" << ENDL;
    return false;
  }

  if (args.size() > 1 && !Common::fromString(args[1], end_index)) {
    std::cout << "wrong end block index parameter" << ENDL;
    return false;
  }

  if (end_index == 0) {
    end_index = end_block_parametr;
  }

  if (end_index > end_block_parametr) {
    std::cout << "end block index parameter shouldn't be greater than " << end_block_parametr << ENDL;
    return false;
  }

  if (end_index <= start_index) {
    std::cout << "end block index should be greater than starter block index" << ENDL;
    return false;
  }

  m_core.print_blockchain(start_index, end_index);
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_height(const std::vector<std::string> &args) {
  logger(Logging::INFO) << "Height: " << m_core.get_current_blockchain_height() << std::endl;
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_bci(const std::vector<std::string>& args)
{
  m_core.print_blockchain_index();
  return true;
}

bool DaemonCommandsHandler::set_log(const std::vector<std::string>& args)
{
  if (args.size() != 1) {
    std::cout << "use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }

  uint16_t l = 0;
  if (!Common::fromString(args[0], l)) {
    std::cout << "wrong number format, use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }

  ++l;

  if (l > Logging::TRACE) {
    std::cout << "wrong number range, use: set_log <log_level_number_0-4>" << ENDL;
    return true;
  }

  m_logManager.setMaxLevel(static_cast<Logging::Level>(l));
  return true;
}

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_block_by_height(uint32_t height)
{
  std::list<CryptoNote::Block> blocks;
  m_core.get_blocks(height, 1, blocks);

  if (1 == blocks.size()) {
    std::cout << "block_id: " << get_block_hash(blocks.front()) << ENDL;
    print_as_json(blocks.front());
  } else {
    uint32_t current_height;
    Crypto::Hash top_id;
    m_core.get_blockchain_top(current_height, top_id);
    std::cout << "block wasn't found. Current block chain height: " << current_height << ", requested: " << height << std::endl;
    return false;
  }

  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_block_by_hash(const std::string& arg)
{
  Crypto::Hash block_hash;
  if (!parse_hash256(arg, block_hash)) {
    return false;
  }

  std::list<Crypto::Hash> block_ids;
  block_ids.push_back(block_hash);
  std::list<CryptoNote::Block> blocks;
  std::list<Crypto::Hash> missed_ids;
  m_core.get_blocks(block_ids, blocks, missed_ids);

  if (1 == blocks.size())
  {
    print_as_json(blocks.front());
  } else
  {
    std::cout << "block wasn't found: " << arg << std::endl;
    return false;
  }

  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_block(const std::vector<std::string> &args) {
  if (args.empty()) {
    std::cout << "expected: print_block (<block_hash> | <block_height>)" << std::endl;
    return true;
  }

  const std::string &arg = args.front();
  try {
    uint32_t height = boost::lexical_cast<uint32_t>(arg);
    print_block_by_height(height);
  } catch (boost::bad_lexical_cast &) {
    print_block_by_hash(arg);
  }

  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_tx(const std::vector<std::string>& args)
{
  if (args.empty()) {
    std::cout << "expected: print_tx <transaction hash>" << std::endl;
    return true;
  }

  const std::string &str_hash = args.front();
  Crypto::Hash tx_hash;
  if (!parse_hash256(str_hash, tx_hash)) {
    return true;
  }

  std::vector<Crypto::Hash> tx_ids;
  tx_ids.push_back(tx_hash);
  std::list<CryptoNote::Transaction> txs;
  std::list<Crypto::Hash> missed_ids;
  m_core.getTransactions(tx_ids, txs, missed_ids, true);

  if (1 == txs.size()) {
    print_as_json(txs.front());
  } else {
    std::cout << "transaction wasn't found: <" << str_hash << '>' << std::endl;
  }

  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_pool(const std::vector<std::string>& args)
{
  logger(Logging::INFO) << "Pool state: " << ENDL << m_core.print_pool(false);
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_pool_sh(const std::vector<std::string>& args)
{
  logger(Logging::INFO) << "Pool state: " << ENDL << m_core.print_pool(true);
  return true;
}
//--------------------------------------------------------------------------------
/*
bool DaemonCommandsHandler::start_dynexchip(const std::vector<std::string> &args) {
  std::cout << "Starting Dynex Chip(s)... " << std::endl;

  if (!args.size()) {
    std::cout << "Please, specify wallet address to receive DNX for: start_dynexchip <addr> [threads=1]" << std::endl;
    return true;
  }

  CryptoNote::AccountPublicAddress adr;
  if (!m_core.currency().parseAccountAddressString(args.front(), adr)) {
    std::cout << "target account address has wrong format" << std::endl;
    return true;
  }

  size_t threads_count = 1;
  if (args.size() > 1) {
    bool ok = Common::fromString(args[1], threads_count);
    threads_count = (ok && 0 < threads_count) ? threads_count : 1;
  }

  uint64_t dynex_minute_rate = 100000; //default fee
  if (args.size() > 2) {
    bool ok = Common::fromString(args[2], dynex_minute_rate);
  }

  //check:
  //auto key = adr.viewPublicKey;
  //std::cout << Common::toHex(&key, sizeof(key)) << std::endl;

  //<== START DYNEXCHIP HERE WHEN INVOICED FROM DAEMON COMMAND LINE
  m_core.m_dynexchip.start(adr, threads_count, dynex_minute_rate);
  
  return true;
}
*/
//--------------------------------------------------------------------------------
/*
bool DaemonCommandsHandler::stop_dynexchip(const std::vector<std::string>& args) {
  //<== STOP DYNEXCHIP HERE WHEN INVOICED FROM DAEMON COMMAND LINE
  //m_core.m_dynexchip.stop();
  
  return true;
}
*/
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_diff(const std::vector<std::string>& args)
{
  logger(Logging::INFO) << "Difficulty for next block: " << m_core.getNextBlockDifficulty() << std::endl;
  return true;
}
//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_pool_count(const std::vector<std::string>& args)
{
  logger(Logging::INFO) << "Pending transactions in mempool: " << m_core.get_pool_transactions_count() << std::endl;
  return true;
}
//--------------------------------------------------------------------------------
/*bool DaemonCommandsHandler::start_mining(const std::vector<std::string> &args) {

  if (!args.size()) {
    std::cout << "Please, specify wallet address to mine for: start_mining <addr> [threads=1]" << std::endl;
    return true;
  }

  CryptoNote::AccountPublicAddress adr;
  if (!m_core.currency().parseAccountAddressString(args.front(), adr)) {
    std::cout << "target account address has wrong format" << std::endl;
    return true;
  }

  size_t threads_count = 1;
  if (args.size() > 1) {
    bool ok = Common::fromString(args[1], threads_count);
    threads_count = (ok && 0 < threads_count) ? threads_count : 1;
  }

  m_core.get_miner().start(adr, threads_count);
  
  return true;
}
*/

//--------------------------------------------------------------------------------
/*
bool DaemonCommandsHandler::stop_mining(const std::vector<std::string>& args) {
  //m_core.get_miner().stop();
  return true;
}
*/

//--------------------------------------------------------------------------------
bool DaemonCommandsHandler::print_ban(const std::vector<std::string>& args) {
  m_srv.log_banlist();
  return true;
}

bool DaemonCommandsHandler::ban(const std::vector<std::string>& args)
{
  if (args.size() != 1 && args.size() != 2) return false;
  std::string addr = args[0];
  uint32_t ip;
  time_t seconds = 3600; // 1h
  if (args.size() > 1) {
    try {
      seconds = std::stoi(args[1]);
    } catch (const std::exception &e) {
      std::cout << "Incorrect time value: " << e.what() << std::endl;
      return false;
    }
    if (seconds == 0) {
      return false;
    }
  }
  try {
    ip = Common::stringToIpAddress(addr);
  } catch (const std::exception &e) {
    std::cout << "Incorrect IP value: " << e.what() << std::endl;
    return false;
  }
  return m_srv.ban_host(ip, seconds);
}

bool DaemonCommandsHandler::unban(const std::vector<std::string>& args)
{
  if (args.size() != 1) return false;
  std::string addr = args[0];
  uint32_t ip;
  try {
    ip = Common::stringToIpAddress(addr);
  }	catch (const std::exception &e) {
    std::cout << "Incorrect IP value: " << e.what() << std::endl;
    return false;
  }
  return m_srv.unban_host(ip);
}
