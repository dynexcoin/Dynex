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


#include "LoggerManager.h"
#include <thread>
#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace Logging {

using Common::JsonValue;

LoggerManager::LoggerManager() {
}

void LoggerManager::operator()(const std::string& category, Level level, boost::posix_time::ptime time, const std::string& body) {
  std::unique_lock<std::mutex> lock(reconfigureLock);
  LoggerGroup::operator()(category, level, time, body);
}

void LoggerManager::configure(const JsonValue& val) {
  std::unique_lock<std::mutex> lock(reconfigureLock);
  loggers.clear();
  LoggerGroup::loggers.clear();
  Level globalLevel;
  if (val.contains("globalLevel")) {
    auto levelVal = val("globalLevel");
    if (levelVal.isInteger()) {
      globalLevel = static_cast<Level>(levelVal.getInteger());
    } else {
      throw std::runtime_error("parameter globalLevel has wrong type");
    }
  } else {
    globalLevel = TRACE;
  }
  std::vector<std::string> globalDisabledCategories;

  if (val.contains("globalDisabledCategories")) {
    auto globalDisabledCategoriesList = val("globalDisabledCategories");
    if (globalDisabledCategoriesList.isArray()) {
      size_t countOfCategories = globalDisabledCategoriesList.size();
      for (size_t i = 0; i < countOfCategories; ++i) {
        auto categoryVal = globalDisabledCategoriesList[i];
        if (categoryVal.isString()) {
          globalDisabledCategories.push_back(categoryVal.getString());
        }
      }
    } else {
      throw std::runtime_error("parameter globalDisabledCategories has wrong type");
    }
  }

  if (val.contains("loggers")) {
    auto loggersList = val("loggers");
    if (loggersList.isArray()) {
      size_t countOfLoggers = loggersList.size();
      for (size_t i = 0; i < countOfLoggers; ++i) {
        auto loggerConfiguration = loggersList[i];
        if (!loggerConfiguration.isObject()) {
          throw std::runtime_error("loggers element must be objects");
        }

        Level level = INFO;
        if (loggerConfiguration.contains("level")) {
          level = static_cast<Level>(loggerConfiguration("level").getInteger());
        }

        std::string type = loggerConfiguration("type").getString();
        std::unique_ptr<Logging::CommonLogger> logger;

        if (type == "console") {
          logger.reset(new ConsoleLogger(level));
        } else if (type == "file") {
          std::string filename = loggerConfiguration("filename").getString();
          auto fileLogger = new FileLogger(level);
          fileLogger->init(filename);
          logger.reset(fileLogger);
        } else {
          throw std::runtime_error("Unknown logger type: " + type);
        }

        if (loggerConfiguration.contains("pattern")) {
          logger->setPattern(loggerConfiguration("pattern").getString());
        }

        std::vector<std::string> disabledCategories;
        if (loggerConfiguration.contains("disabledCategories")) {
          auto disabledCategoriesVal = loggerConfiguration("disabledCategories");
          size_t countOfCategories = disabledCategoriesVal.size();
          for (size_t i = 0; i < countOfCategories; ++i) {
            auto categoryVal = disabledCategoriesVal[i];
            if (categoryVal.isString()) {
              logger->disableCategory(categoryVal.getString());
            }
          }
        }

        loggers.emplace_back(std::move(logger));
        addLogger(*loggers.back());
      }
    } else {
      throw std::runtime_error("loggers parameter has wrong type");
    }
  } else {
    throw std::runtime_error("loggers parameter missing");
  }
  setMaxLevel(globalLevel);
  for (const auto& category : globalDisabledCategories) {
    disableCategory(category);
  }
}

}
