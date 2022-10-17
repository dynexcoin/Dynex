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

#include "CryptoNote.h"
#include <Common/MemoryInputStream.h>
#include <Common/VectorOutputStream.h>
#include "Serialization/KVBinaryInputStreamSerializer.h"
#include "Serialization/KVBinaryOutputStreamSerializer.h"

namespace System {
class TcpConnection;
}

namespace CryptoNote {

enum class LevinError: int32_t {
  OK = 0,
  ERROR_CONNECTION = -1,
  ERROR_CONNECTION_NOT_FOUND = -2,
  ERROR_CONNECTION_DESTROYED = -3,
  ERROR_CONNECTION_TIMEDOUT = -4,
  ERROR_CONNECTION_NO_DUPLEX_PROTOCOL = -5,
  ERROR_CONNECTION_HANDLER_NOT_DEFINED = -6,
  ERROR_FORMAT = -7,
};

const int32_t LEVIN_PROTOCOL_RETCODE_SUCCESS = 1;

class LevinProtocol {
public:

  LevinProtocol(System::TcpConnection& connection);

  template <typename Request, typename Response>
  bool invoke(uint32_t command, const Request& request, Response& response) {
    sendMessage(command, encode(request), true);

    Command cmd;
    readCommand(cmd);

    if (!cmd.isResponse) {
      return false;
    }

    return decode(cmd.buf, response); 
  }

  template <typename Request>
  void notify(uint32_t command, const Request& request, int) {
    sendMessage(command, encode(request), false);
  }

  struct Command {
    uint32_t command;
    bool isNotify;
    bool isResponse;
    BinaryArray buf;

    bool needReply() const;
  };

  bool readCommand(Command& cmd);

  void sendMessage(uint32_t command, const BinaryArray& out, bool needResponse);
  void sendReply(uint32_t command, const BinaryArray& out, int32_t returnCode);

  template <typename T>
  static bool decode(const BinaryArray& buf, T& value) {
    try {
      Common::MemoryInputStream stream(buf.data(), buf.size());
      KVBinaryInputStreamSerializer serializer(stream);
      serialize(value, serializer);
    } catch (std::exception&) {
      return false;
    }

    return true;
  }

  template <typename T>
  static BinaryArray encode(const T& value) {
    BinaryArray result;
    KVBinaryOutputStreamSerializer serializer;
    serialize(const_cast<T&>(value), serializer);
    Common::VectorOutputStream stream(result);
    serializer.dump(stream);
    return result;
  }

private:

  bool readStrict(uint8_t* ptr, size_t size);
  void writeStrict(const uint8_t* ptr, size_t size);
  System::TcpConnection& m_conn;
};

}
