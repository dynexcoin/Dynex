
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



#include <stdint.h>
#include <stdio.h>
#include <string>
#include <boost/regex.hpp>
#include "CryptoNoteConfig.h"


namespace Common {

const uint16_t HTTP_PORT = 80;
const uint16_t HTTPS_PORT = 443;
const std::string RPC_PATH = "/";

bool parseUrlAddress(const std::string& url, std::string& host, uint16_t& port, std::string& path, bool& ssl) {
  bool res = true;

  host.clear();
  path.clear();
  port = 0;
  ssl = false;

  boost::regex uri_exp("^(https://|http://|)(([a-z|A-Z|0-9]|[a-z|A-Z|0-9]-[a-z|A-Z|0-9]|[a-z|A-Z|0-9]\\.)+)(:[0-9]{1,5}|)(/([\\w|-]+/)+|/|)$");
  boost::cmatch reg_res;
  if (boost::regex_match(url.c_str(), reg_res, uri_exp)) {
    if (reg_res.length(4) > 0) {
      int port_src = 0;
      if (sscanf(reg_res.str(4).c_str() + 1, "%d", &port_src) == 1) {
        if (port_src > 0 && port_src <= 0xFFFF) port = (uint16_t) port_src;
      }
    } else {
      if (strcmp(reg_res.str(1).c_str(), "http://") == 0) port = HTTP_PORT;
      else if (strcmp(reg_res.str(1).c_str(), "https://") == 0) port = HTTPS_PORT;
      else port = CryptoNote::RPC_DEFAULT_PORT;
    }
    if (port != 0) {
      if (strcmp(reg_res.str(1).c_str(), "https://") == 0) ssl = true;
      host.assign(reg_res[2].first, reg_res[2].second);
      path.assign(reg_res[5].first, reg_res[5].second);
      if (path.empty()) path = RPC_PATH;
    } else {
      res = false;
    }
  } else {
    res = false;
  }

  return res;
}

}

