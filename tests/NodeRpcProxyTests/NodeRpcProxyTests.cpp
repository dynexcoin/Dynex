// Copyright (c) 2021-2022, The TuringX Project
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
// Parts of this file are originally copyright (c) 2012-2016 The Cryptonote developers

#include <chrono>
#include <thread>

#include <Logging/LoggerRef.h>
#include <Logging/ConsoleLogger.h>

#include "NodeRpcProxy/NodeRpcProxy.h"

using namespace CryptoNote;
using namespace Logging;

#undef ERROR

class NodeObserver : public INodeObserver {
public:
  NodeObserver(const std::string& name, NodeRpcProxy& nodeProxy, ILogger& log)
    : m_name(name)
    , m_nodeProxy(nodeProxy)
    , logger(log, "NodeObserver:" + name) {
  }

  virtual ~NodeObserver() {
  }

  virtual void peerCountUpdated(size_t count) override {
    logger(INFO) << '[' << m_name << "] peerCountUpdated " << count << " = " << m_nodeProxy.getPeerCount();
  }

  virtual void localBlockchainUpdated(uint32_t height) override {
    logger(INFO) << '[' << m_name << "] localBlockchainUpdated " << height << " = " << m_nodeProxy.getLastLocalBlockHeight();

    std::vector<uint64_t> amounts;
    amounts.push_back(100000000);
    auto outs = std::make_shared<std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>>();
    m_nodeProxy.getRandomOutsByAmounts(std::move(amounts), 10, *outs.get(), [outs, this](std::error_code ec) {
      if (!ec) {
        if (1 == outs->size() && 10 == (*outs)[0].outs.size()) {
          logger(INFO) << "getRandomOutsByAmounts called successfully";
        } else {
          logger(ERROR) << "getRandomOutsByAmounts returned invalid result";
        }
      } else {
        logger(ERROR) << "failed to call getRandomOutsByAmounts: " << ec.message() << ':' << ec.value();
      }
    });
  }

  virtual void lastKnownBlockHeightUpdated(uint32_t height) override {
    logger(INFO) << '[' << m_name << "] lastKnownBlockHeightUpdated " << height << " = " << m_nodeProxy.getLastKnownBlockHeight();
  }

private:
  LoggerRef logger;
  std::string m_name;
  NodeRpcProxy& m_nodeProxy;
};

int main(int argc, const char** argv) {

  Logging::ConsoleLogger log;
  Logging::LoggerRef logger(log, "main");
  NodeRpcProxy nodeProxy("127.0.0.1", 18081);

  NodeObserver observer1("obs1", nodeProxy, log);
  NodeObserver observer2("obs2", nodeProxy, log);

  nodeProxy.addObserver(&observer1);
  nodeProxy.addObserver(&observer2);

  nodeProxy.init([&](std::error_code ec) {
    if (ec) {
      logger(ERROR) << "init error: " << ec.message() << ':' << ec.value();
    } else {
      logger(INFO, BRIGHT_GREEN) << "initialized";
    }
  });

  //nodeProxy.init([](std::error_code ec) {
  //  if (ec) {
  //    logger(ERROR) << "init error: " << ec.message() << ':' << ec.value();
  //  } else {
  //    LOG_PRINT_GREEN("initialized", LOG_LEVEL_0);
  //  }
  //});

  std::this_thread::sleep_for(std::chrono::seconds(5));
  if (nodeProxy.shutdown()) {
    logger(INFO, BRIGHT_GREEN) << "shutdown";
  } else {
    logger(ERROR) << "shutdown error";
  }

  nodeProxy.init([&](std::error_code ec) {
    if (ec) {
      logger(ERROR) << "init error: " << ec.message() << ':' << ec.value();
    } else {
      logger(INFO, BRIGHT_GREEN) << "initialized";
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(5));
  if (nodeProxy.shutdown()) {
    logger(INFO, BRIGHT_GREEN) << "shutdown";
  } else {
    logger(ERROR) << "shutdown error";
  }

  CryptoNote::Transaction tx;
  nodeProxy.relayTransaction(tx, [&](std::error_code ec) {
    if (!ec) {
      logger(INFO) << "relayTransaction called successfully";
    } else {
      logger(ERROR) << "failed to call relayTransaction: " << ec.message() << ':' << ec.value();
    }
  });

  nodeProxy.init([&](std::error_code ec) {
    if (ec) {
      logger(ERROR) << "init error: " << ec.message() << ':' << ec.value();
    } else {
      logger(INFO, BRIGHT_GREEN) << "initialized";
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(5));
  nodeProxy.relayTransaction(tx, [&](std::error_code ec) {
    if (!ec) {
      logger(INFO) << "relayTransaction called successfully";
    } else {
      logger(ERROR) << "failed to call relayTransaction: " << ec.message() << ':' << ec.value();
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(60));
}
