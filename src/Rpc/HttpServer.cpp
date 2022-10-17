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


#include "HttpServer.h"
#include <boost/scope_exit.hpp>

#include <Common/Base64.h>
#include <HTTP/HttpParser.h>
#include <System/InterruptedException.h>
#include <System/TcpStream.h>
#include <System/Ipv4Address.h>

using namespace Logging;

namespace {
	void fillUnauthorizedResponse(CryptoNote::HttpResponse& response) {
		response.setStatus(CryptoNote::HttpResponse::STATUS_401);
		response.addHeader("WWW-Authenticate", "Basic realm=\"RPC\"");
		response.addHeader("Content-Type", "text/plain");
		response.setBody("Authorization required");
	}
}

namespace CryptoNote {

HttpServer::HttpServer(System::Dispatcher& dispatcher, Logging::ILogger& log)
  : m_dispatcher(dispatcher), workingContextGroup(dispatcher), logger(log, "HttpServer") {

}

void HttpServer::start(const std::string& address, uint16_t port, const std::string& user, const std::string& password) {
  m_listener = System::TcpListener(m_dispatcher, System::Ipv4Address(address), port);
  workingContextGroup.spawn(std::bind(&HttpServer::acceptLoop, this));
  
  		if (!user.empty() || !password.empty()) {
			m_credentials = Tools::Base64::encode(user + ":" + password);
		}
}

void HttpServer::stop() {
  workingContextGroup.interrupt();
  workingContextGroup.wait();
}

void HttpServer::acceptLoop() {
  try {
    System::TcpConnection connection; 
    bool accepted = false;

    while (!accepted) {
      try {
        connection = m_listener.accept();
        accepted = true;
      } catch (System::InterruptedException&) {
        throw;
      } catch (std::exception&) {
        // try again
      }
    }

    m_connections.insert(&connection);
    BOOST_SCOPE_EXIT_ALL(this, &connection) { 
      m_connections.erase(&connection); };

	workingContextGroup.spawn(std::bind(&HttpServer::acceptLoop, this));

	//auto addr = connection.getPeerAddressAndPort();
	auto addr = std::pair<System::Ipv4Address, uint16_t>(static_cast<System::Ipv4Address>(0), 0);
	try {
		addr = connection.getPeerAddressAndPort();
	} catch (std::runtime_error&) {
		logger(WARNING) << "Could not get IP of connection";
	}

    logger(DEBUGGING) << "Incoming connection from " << addr.first.toDottedDecimal() << ":" << addr.second;

    System::TcpStreambuf streambuf(connection);
    std::iostream stream(&streambuf);
    HttpParser parser;

    for (;;) {
      HttpRequest req;
      HttpResponse resp;
	  resp.addHeader("Access-Control-Allow-Origin", "*");
	  resp.addHeader("content-type", "application/json");
	
      parser.receiveRequest(stream, req);
				if (authenticate(req)) {
					processRequest(req, resp);
				}
				else {
					logger(WARNING) << "Authorization required " << addr.first.toDottedDecimal() << ":" << addr.second;
					fillUnauthorizedResponse(resp);
				}

      stream << resp;
      stream.flush();

      if (stream.peek() == std::iostream::traits_type::eof()) {
        break;
      }
    }

    logger(DEBUGGING) << "Closing connection from " << addr.first.toDottedDecimal() << ":" << addr.second << " total=" << m_connections.size();

  } catch (System::InterruptedException&) {
  } catch (std::exception& e) {
    logger(DEBUGGING) << "Connection error: " << e.what();
  }
}

bool HttpServer::authenticate(const HttpRequest& request) const {
	if (!m_credentials.empty()) {
		auto headerIt = request.getHeaders().find("authorization");
		if (headerIt == request.getHeaders().end()) {
			return false;
		}

		if (headerIt->second.substr(0, 6) != "Basic ") {
			return false;
		}

		if (headerIt->second.substr(6) != m_credentials) {
			return false;
		}
	}

	return true;
}

size_t HttpServer::get_connections_count() const {
	return m_connections.size();
}

}
