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


#include "HttpClient.h"

#include <HTTP/HttpParser.h>
#include <System/Ipv4Resolver.h>
#include <System/Ipv4Address.h>
#include <System/TcpConnector.h>

namespace CryptoNote {

HttpClient::HttpClient(System::Dispatcher& dispatcher, const std::string& address, uint16_t port) :
  m_dispatcher(dispatcher), m_address(address), m_port(port) {
}

HttpClient::~HttpClient() {
  if (m_connected) {
    disconnect();
  }
}

void HttpClient::request(const HttpRequest &req, HttpResponse &res) {
  if (!m_connected) {
    connect();
  }

  try {
    std::iostream stream(m_streamBuf.get());
    HttpParser parser;
    stream << req;
    stream.flush();
    parser.receiveResponse(stream, res);
  } catch (const std::exception &) {
    disconnect();
    throw;
  }
}

void HttpClient::connect() {
  try {
    auto ipAddr = System::Ipv4Resolver(m_dispatcher).resolve(m_address);
    m_connection = System::TcpConnector(m_dispatcher).connect(ipAddr, m_port);
    m_streamBuf.reset(new System::TcpStreambuf(m_connection));
    m_connected = true;
  } catch (const std::exception& e) {
    throw ConnectException(e.what());
  }
}

bool HttpClient::isConnected() const {
  return m_connected;
}

void HttpClient::disconnect() {
  m_streamBuf.reset();
  try {
    m_connection.write(nullptr, 0); //Socket shutdown.
  } catch (std::exception&) {
    //Ignoring possible exception.
  }

  try {
    m_connection = System::TcpConnection();
  } catch (std::exception&) {
    //Ignoring possible exception.
  }

  m_connected = false;
}

ConnectException::ConnectException(const std::string& whatArg) : std::runtime_error(whatArg.c_str()) {
}

}
