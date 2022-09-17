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

#include <stdexcept>
#include "Ipv4Resolver.h"
#include <cassert>
#include <random>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <ws2tcpip.h>
#include <System/Dispatcher.h>
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/Ipv4Address.h>

namespace System {

Ipv4Resolver::Ipv4Resolver() : dispatcher(nullptr) {
}

Ipv4Resolver::Ipv4Resolver(Dispatcher& dispatcher) : dispatcher(&dispatcher) {
}

Ipv4Resolver::Ipv4Resolver(Ipv4Resolver&& other) : dispatcher(other.dispatcher) {
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }
}

Ipv4Resolver::~Ipv4Resolver() {
}

Ipv4Resolver& Ipv4Resolver::operator=(Ipv4Resolver&& other) {
  dispatcher = other.dispatcher;
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }

  return *this;
}

Ipv4Address Ipv4Resolver::resolve(const std::string& host) {
  assert(dispatcher != nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  addrinfo hints = { 0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL };
  addrinfo* addressInfos;
  int result = getaddrinfo(host.c_str(), NULL, &hints, &addressInfos);
  if (result != 0) {
    throw std::runtime_error("Ipv4Resolver::resolve, getaddrinfo failed, " + errorMessage(result));
  }
  
  size_t count = 0;
  for (addrinfo* addressInfo = addressInfos; addressInfo != nullptr; addressInfo = addressInfo->ai_next) {
    ++count;
  }

  std::mt19937 generator{ std::random_device()() };
  size_t index = std::uniform_int_distribution<size_t>(0, count - 1)(generator);
  addrinfo* addressInfo = addressInfos;
  for (size_t i = 0; i < index; ++i) {
    addressInfo = addressInfo->ai_next;
  }

  Ipv4Address address(ntohl(reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr)->sin_addr.S_un.S_addr));
  freeaddrinfo(addressInfo);
  return address;
}

}
