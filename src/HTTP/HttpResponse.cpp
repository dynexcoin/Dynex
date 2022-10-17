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


#include "HttpResponse.h"

#include <stdexcept>

namespace {

const char* getStatusString(CryptoNote::HttpResponse::HTTP_STATUS status) {
  switch (status) {
  case CryptoNote::HttpResponse::STATUS_200:
    return "200 OK";
  case CryptoNote::HttpResponse::STATUS_401:
    return "401 Unauthorized";
  case CryptoNote::HttpResponse::STATUS_404:
    return "404 Not Found";
  case CryptoNote::HttpResponse::STATUS_500:
    return "500 Internal Server Error";
  default:
    throw std::runtime_error("Unknown HTTP status code is given");
  }

  return ""; //unaccessible
}

const char* getErrorBody(CryptoNote::HttpResponse::HTTP_STATUS status) {
  switch (status) {
  case CryptoNote::HttpResponse::STATUS_401:
    return "Authorization required\n";
  case CryptoNote::HttpResponse::STATUS_404:
    return "Requested url is not found\n";
  case CryptoNote::HttpResponse::STATUS_500:
    return "Internal server error is occurred\n";
  default:
    throw std::runtime_error("Error body for given status is not available");
  }

  return ""; //unaccessible
}

} //namespace

namespace CryptoNote {

HttpResponse::HttpResponse() {
  status = STATUS_200;
  headers["Server"] = "CryptoNote-based HTTP server";
  headers["Access-Control-Allow-Origin"] = "*";
}

void HttpResponse::setStatus(HTTP_STATUS s) {
  status = s;

  if (status != HttpResponse::STATUS_200) {
    setBody(getErrorBody(status));
  }
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
  headers[name] = value;
}

void HttpResponse::setBody(const std::string& b) {
  body = b;
  if (!body.empty()) {
    headers["Content-Length"] = std::to_string(body.size());
  } else {
    headers.erase("Content-Length");
  }
}

std::ostream& HttpResponse::printHttpResponse(std::ostream& os) const {
  os << "HTTP/1.1 " << getStatusString(status) << "\r\n";

  for (auto pair: headers) {
    os << pair.first << ": " << pair.second << "\r\n";
  }
  os << "\r\n";

  if (!body.empty()) {
    os << body;
  }

  return os;
}

} //namespace CryptoNote
