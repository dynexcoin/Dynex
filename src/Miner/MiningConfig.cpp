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


#include "MiningConfig.h"

#include <iostream>
#include <thread>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "CryptoNoteConfig.h"
#include "Logging/ILogger.h"

namespace po = boost::program_options;

namespace CryptoNote {

namespace {

const size_t DEFAULT_SCANT_PERIOD = 30;
const char* DEFAULT_DAEMON_HOST = "127.0.0.1";
const size_t CONCURRENCY_LEVEL = std::thread::hardware_concurrency();

po::options_description cmdOptions;

void parseDaemonAddress(const std::string& daemonAddress, std::string& daemonHost, uint16_t& daemonPort) {
  std::vector<std::string> splittedAddress;
  boost::algorithm::split(splittedAddress, daemonAddress, boost::algorithm::is_any_of(":"));

  if (splittedAddress.size() != 2) {
    throw std::runtime_error("Wrong daemon address format");
  }

  if (splittedAddress[0].empty() || splittedAddress[1].empty()) {
    throw std::runtime_error("Wrong daemon address format");
  }

  daemonHost = splittedAddress[0];

  try {
    daemonPort = boost::lexical_cast<uint16_t>(splittedAddress[1]);
  } catch (std::exception&) {
    throw std::runtime_error("Wrong daemon address format");
  }
}

}

MiningConfig::MiningConfig(): help(false) {
  cmdOptions.add_options()
      ("help,h", "produce this help message and exit")
      ("address", po::value<std::string>(), "Valid cryptonote miner's address")
      ("daemon-host", po::value<std::string>()->default_value(DEFAULT_DAEMON_HOST), "Daemon host")
      ("daemon-rpc-port", po::value<uint16_t>()->default_value(static_cast<uint16_t>(RPC_DEFAULT_PORT)), "Daemon's RPC port")
      ("daemon-address", po::value<std::string>(), "Daemon host:port. If you use this option you must not use --daemon-host and --daemon-port options")
      ("threads", po::value<size_t>()->default_value(CONCURRENCY_LEVEL), "Mining threads count. Must not be greater than you concurrency level. Default value is your hardware concurrency level")
      ("scan-time", po::value<size_t>()->default_value(DEFAULT_SCANT_PERIOD), "Blockchain polling interval (seconds). How often miner will check blockchain for updates")
      ("log-level", po::value<int>()->default_value(1), "Log level. Must be 0..5")
      ("limit", po::value<size_t>()->default_value(0), "Mine exact quantity of blocks. 0 means no limit")
      ("first-block-timestamp", po::value<uint64_t>()->default_value(0), "Set timestamp to the first mined block. 0 means leave timestamp unchanged")
      ("block-timestamp-interval", po::value<int64_t>()->default_value(0), "Timestamp step for each subsequent block. May be set only if --first-block-timestamp has been set."
                                                         " If not set blocks' timestamps remain unchanged");
}

void MiningConfig::parse(int argc, char** argv) {
  po::variables_map options;
  po::store(po::parse_command_line(argc, argv, cmdOptions), options);
  po::notify(options);

  if (options.count("help") != 0) {
    help = true;
    return;
  }

  if (options.count("address") == 0) {
    throw std::runtime_error("Specify --address option");
  }

  miningAddress = options["address"].as<std::string>();

  if (!options["daemon-address"].empty()) {
    if (!options["daemon-host"].defaulted() || !options["daemon-rpc-port"].defaulted()) {
      throw std::runtime_error("Either --daemon-host or --daemon-rpc-port is already specified. You must not specify --daemon-address");
    }

    parseDaemonAddress(options["daemon-address"].as<std::string>(), daemonHost, daemonPort);
  } else {
    daemonHost = options["daemon-host"].as<std::string>();
    daemonPort = options["daemon-rpc-port"].as<uint16_t>();
  }

  threadCount = options["threads"].as<size_t>();
  if (threadCount == 0 || threadCount > CONCURRENCY_LEVEL) {
    throw std::runtime_error("--threads option must be 1.." + std::to_string(CONCURRENCY_LEVEL));
  }

  scanPeriod = options["scan-time"].as<size_t>();
  if (scanPeriod == 0) {
    throw std::runtime_error("--scan-time must not be zero");
  }

  logLevel = static_cast<uint8_t>(options["log-level"].as<int>());
  if (logLevel > static_cast<uint8_t>(Logging::TRACE)) {
    throw std::runtime_error("--log-level value is too big");
  }

  blocksLimit = options["limit"].as<size_t>();

  if (!options["block-timestamp-interval"].defaulted() && options["first-block-timestamp"].defaulted()) {
    throw std::runtime_error("If you specify --block-timestamp-interval you must specify --first-block-timestamp either");
  }

  firstBlockTimestamp = options["first-block-timestamp"].as<uint64_t>();
  blockTimestampInterval = options["block-timestamp-interval"].as<int64_t>();
}

void MiningConfig::printHelp() {
  std::cout << cmdOptions << std::endl;
}

}
