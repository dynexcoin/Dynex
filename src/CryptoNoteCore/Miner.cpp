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


#include "Miner.h"
#include <future>
#include <numeric>
#include <sstream>
#include <thread>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <boost/limits.hpp>
#include <boost/utility/value_init.hpp>
#include "crypto/crypto.h"
#include "Common/CommandLine.h"
#include "Common/StringTools.h"
#include "Serialization/SerializationTools.h"
#include "CryptoNoteFormatUtils.h"
#include "TransactionExtra.h"

using namespace Logging;

namespace CryptoNote
{

  miner::miner(const Currency& currency, IMinerHandler& handler, Logging::ILogger& log) :
    m_currency(currency),
    logger(log, "miner"),
    m_stop(true),
    m_template(boost::value_initialized<Block>()),
    m_template_no(0),
    m_diffic(0),
    m_handler(handler),
    m_pausers_count(0),
    m_threads_total(0),
    m_starter_nonce(0),
    m_last_hr_merge_time(0),
    m_hashes(0),
    m_do_print_hashrate(false),
    m_do_mining(false),
    m_current_hash_rate(0),
    m_update_block_template_interval(5),
    m_update_merge_hr_interval(2)
  {
  }
  //-----------------------------------------------------------------------------------------------------
  miner::~miner() {
    stop();
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_block_template(const Block& bl, const difficulty_type& di, uint32_t& height) {
    std::lock_guard<decltype(m_template_lock)> lk(m_template_lock);
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_block_chain_update() {
    return false;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::request_block_template() {
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_idle(){
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::do_print_hashrate(bool do_hr)
  {
    m_do_print_hashrate = do_hr;
  }

  uint64_t millisecondsSinceEpoch() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  }

  //-----------------------------------------------------------------------------------------------------
  void miner::merge_hr(){}

  bool miner::init(const MinerConfig& config) {
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::is_mining()  {
    return !m_stop;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::start(const AccountPublicAddress& adr, size_t threads_count, std::string mallob_network_id, uint64_t mallob_speed)  {
    return true;
  }
  
  //-----------------------------------------------------------------------------------------------------
  uint64_t miner::get_speed()  {
    return 0;
  }
  
  //-----------------------------------------------------------------------------------------------------
  void miner::send_stop_signal()   {
    m_stop = true;
  }

  //-----------------------------------------------------------------------------------------------------
  bool miner::stop()  {
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::find_nonce_for_given_block(Crypto::cn_context &context, Block& bl, const difficulty_type& diffic) {
    return false;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::on_synchronized(){}
  //-----------------------------------------------------------------------------------------------------
  void miner::pause(){}
  //-----------------------------------------------------------------------------------------------------
  void miner::resume(){}
  //-----------------------------------------------------------------------------------------------------
  bool miner::worker_thread(uint32_t th_local_index){
   return true;
  }
  //-----------------------------------------------------------------------------------------------------
}
