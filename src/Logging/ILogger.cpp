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


#include "ILogger.h"

namespace Logging {

const std::string BLUE = "\x1F""BLUE\x1F";
const std::string GREEN = "\x1F""GREEN\x1F";
const std::string RED = "\x1F""RED\x1F";
const std::string YELLOW = "\x1F""YELLOW\x1F";
const std::string WHITE = "\x1F""WHITE\x1F";
const std::string CYAN = "\x1F""CYAN\x1F";
const std::string MAGENTA = "\x1F""MAGENTA\x1F";
const std::string BRIGHT_BLUE = "\x1F""BRIGHT_BLUE\x1F";
const std::string BRIGHT_GREEN = "\x1F""BRIGHT_GREEN\x1F";
const std::string BRIGHT_RED = "\x1F""BRIGHT_RED\x1F";
const std::string BRIGHT_YELLOW = "\x1F""BRIGHT_YELLOW\x1F";
const std::string BRIGHT_WHITE = "\x1F""BRIGHT_WHITE\x1F";
const std::string BRIGHT_CYAN = "\x1F""BRIGHT_CYAN\x1F";
const std::string BRIGHT_MAGENTA = "\x1F""BRIGHT_MAGENTA\x1F";
const std::string DEFAULT = "\x1F""DEFAULT\x1F";

const char ILogger::COLOR_DELIMETER = '\x1F';

const std::array<std::string, 6> ILogger::LEVEL_NAMES = {
  {"FATAL",
  "ERROR",
  "WARNING",
  "INFO",
  "DEBUG",
  "TRACE"}
};

}
