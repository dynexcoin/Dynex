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


#pragma once

#include <memory>

#include <Common/Base64.h>
#include <HTTP/HttpRequest.h>
#include <HTTP/HttpResponse.h>
#include <System/TcpConnection.h>
#include <System/TcpStream.h>
#include "JsonRpc.h"

#include "Serialization/SerializationTools.h"

namespace CryptoNote {

class ConnectException : public std::runtime_error  {
public:
  ConnectException(const std::string& whatArg);
};

class HttpClient {
public:

  HttpClient(System::Dispatcher& dispatcher, const std::string& address, uint16_t port);
  ~HttpClient();
  void request(const HttpRequest& req, HttpResponse& res);
  
  bool isConnected() const;

private:
  void connect();
  void disconnect();

  const std::string m_address;
  const uint16_t m_port;

  bool m_connected = false;
  System::Dispatcher& m_dispatcher;
  System::TcpConnection m_connection;
  std::unique_ptr<System::TcpStreambuf> m_streamBuf;
};

template <typename Request, typename Response>
  void invokeJsonCommand(HttpClient& client, const std::string& url, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
//  void invokeJsonCommand(HttpClient& client, const std::string& url, const Request& req, Response& res) {    
  HttpRequest hreq;
  HttpResponse hres;

  hreq.addHeader("Content-Type", "application/json");
/*
  if (!user.empty() || !password.empty()) {
    hreq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
  }
*/
  hreq.setUrl(url);
  hreq.setBody(storeToJson(req));
  client.request(hreq, hres);

  if (hres.getStatus() != HttpResponse::STATUS_200) {
    throw std::runtime_error("HTTP status: " + std::to_string(hres.getStatus()));
  }

  if (!loadFromJson(res, hres.getBody())) {
    throw std::runtime_error("Failed to parse JSON response");
  }
}

template <typename Request, typename Response>
void invokeJsonRpcCommand(HttpClient& client, const std::string& method, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
  try {

    JsonRpc::JsonRpcRequest jsReq;

    jsReq.setMethod(method);
    jsReq.setParams(req);

    HttpRequest httpReq;
    HttpResponse httpRes;

    httpReq.addHeader("Content-Type", "application/json");
    if (!user.empty() || !password.empty()) {
      httpReq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
    }
    httpReq.setUrl("/json_rpc");
    httpReq.setBody(jsReq.getBody());

    client.request(httpReq, httpRes);

    JsonRpc::JsonRpcResponse jsRes;

    //if (httpRes.getStatus() == HttpResponse::STATUS_200) {
      jsRes.parse(httpRes.getBody());
      if (!jsRes.getResult(res)) {
        throw std::runtime_error("HTTP status: " + std::to_string(httpRes.getStatus()));
      }
    //}

  } catch (const ConnectException&) {
    throw std::runtime_error("HTTP status: CONNECT_ERROR");
  } catch (const std::exception&) {
    throw std::runtime_error("HTTP status: NETWORK_ERROR");
  }
}

template <typename Request, typename Response>

//void invokeBinaryCommand(HttpClient& client, const std::string& url, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
void invokeBinaryCommand(HttpClient& client, const std::string& url, const Request& req, Response& res) {
  HttpRequest hreq;
  HttpResponse hres;

/*
  if (!user.empty() || !password.empty()) {
    hreq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
  }
*/
  hreq.setUrl(url);
  hreq.setBody(storeToBinaryKeyValue(req));
  client.request(hreq, hres);

  if (!loadFromBinaryKeyValue(res, hres.getBody())) {
    throw std::runtime_error("Failed to parse binary response");
  }
}
  
}
