// Copyright (c) 2021-2023, Dynex Developers
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
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Common {

std::string asString(const void* data, size_t size); // Does not throw
std::string asString(const std::vector<uint8_t>& data); // Does not throw
std::vector<uint8_t> asBinaryArray(const std::string& data);

uint8_t fromHex(char character); // Returns value of hex 'character', throws on error
bool fromHex(char character, uint8_t& value); // Assigns value of hex 'character' to 'value', returns false on error, does not throw
size_t fromHex(const std::string& text, void* data, size_t bufferSize); // Assigns values of hex 'text' to buffer 'data' up to 'bufferSize', returns actual data size, throws on error
bool fromHex(const std::string& text, void* data, size_t bufferSize, size_t& size); // Assigns values of hex 'text' to buffer 'data' up to 'bufferSize', assigns actual data size to 'size', returns false on error, does not throw
std::vector<uint8_t> fromHex(const std::string& text); // Returns values of hex 'text', throws on error
bool fromHex(const std::string& text, std::vector<uint8_t>& data); // Appends values of hex 'text' to 'data', returns false on error, does not throw

template <typename T>
bool podFromHex(const std::string& text, T& val) {
  size_t outSize;
  return fromHex(text, &val, sizeof(val), outSize) && outSize == sizeof(val);
}

std::string toHex(const void* data, size_t size); // Returns hex representation of ('data', 'size'), does not throw
void toHex(const void* data, size_t size, std::string& text); // Appends hex representation of ('data', 'size') to 'text', does not throw
std::string toHex(const std::vector<uint8_t>& data); // Returns hex representation of 'data', does not throw
void toHex(const std::vector<uint8_t>& data, std::string& text); // Appends hex representation of 'data' to 'text', does not throw

template<class T>
std::string podToHex(const T& s) {
  return toHex(&s, sizeof(s));
}

std::string extract(std::string& text, char delimiter); // Does not throw
std::string extract(const std::string& text, char delimiter, size_t& offset); // Does not throw

template<typename T> T fromString(const std::string& text) { // Throws on error
  T value;
  std::istringstream stream(text);
  stream >> value;
  if (stream.fail()) {
    throw std::runtime_error("fromString: unable to parse value");
  }

  return value;
}

template<typename T> bool fromString(const std::string& text, T& value) { // Does not throw
  std::istringstream stream(text);
  stream >> value;
  return !stream.fail();
}

template<typename T> std::vector<T> fromDelimitedString(const std::string& source, char delimiter) { // Throws on error
  std::vector<T> data;
  for (size_t offset = 0; offset != source.size();) {
    data.emplace_back(fromString<T>(extract(source, delimiter, offset)));
  }

  return data;
}

template<typename T> bool fromDelimitedString(const std::string& source, char delimiter, std::vector<T>& data) { // Does not throw
  for (size_t offset = 0; offset != source.size();) {
    T value;
    if (!fromString<T>(extract(source, delimiter, offset), value)) {
      return false;
    }

    data.emplace_back(value);
  }

  return true;
}

template<typename T> std::string toString(const T& value) { // Does not throw
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

template<typename T> void toString(const T& value, std::string& text) { // Does not throw
  std::ostringstream stream;
  stream << value;
  text += stream.str();
}

bool loadFileToString(const std::string& filepath, std::string& buf);
bool saveStringToFile(const std::string& filepath, const std::string& buf);


std::string base64Decode(std::string const& encoded_string);

std::string ipAddressToString(uint32_t ip);
uint32_t stringToIpAddress(std::string addr);
bool parseIpAddressAndPort(uint32_t& ip, uint32_t& port, const std::string& addr);

std::string timeIntervalToString(uint64_t intervalInSeconds);

}
