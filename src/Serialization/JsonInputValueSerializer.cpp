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

#include "JsonInputValueSerializer.h"

#include <cassert>
#include <stdexcept>

#include "Common/StringTools.h"

using Common::JsonValue;
using namespace CryptoNote;

JsonInputValueSerializer::JsonInputValueSerializer(const Common::JsonValue& value) {
  if (!value.isObject()) {
    throw std::runtime_error("Serializer doesn't support this type of serialization: Object expected.");
  }

  chain.push_back(&value);
}

JsonInputValueSerializer::JsonInputValueSerializer(Common::JsonValue&& value) : value(std::move(value)) {
  if (!this->value.isObject()) {
    throw std::runtime_error("Serializer doesn't support this type of serialization: Object expected.");
  }

  chain.push_back(&this->value);
}

JsonInputValueSerializer::~JsonInputValueSerializer() {
}

ISerializer::SerializerType JsonInputValueSerializer::type() const {
  return ISerializer::INPUT;
}

bool JsonInputValueSerializer::beginObject(Common::StringView name) {
  const JsonValue* parent = chain.back();

  if (parent->isArray()) {
    const JsonValue& v = (*parent)[idxs.back()++];
    chain.push_back(&v);
    return true;
  }

  if (parent->contains(std::string(name))) {
    const JsonValue& v = (*parent)(std::string(name));
    chain.push_back(&v);
    return true;
  }

  return false;
}

void JsonInputValueSerializer::endObject() {
  assert(!chain.empty());
  chain.pop_back();
}

bool JsonInputValueSerializer::beginArray(size_t& size, Common::StringView name) {
  const JsonValue* parent = chain.back();
  std::string strName(name);

  if (parent->contains(strName)) {
    const JsonValue& arr = (*parent)(strName);
    size = arr.size();
    chain.push_back(&arr);
    idxs.push_back(0);
    return true;
  }
 
  size = 0;
  return false;
}

void JsonInputValueSerializer::endArray() {
  assert(!chain.empty());
  assert(!idxs.empty());

  chain.pop_back();
  idxs.pop_back();
}

bool JsonInputValueSerializer::operator()(uint16_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int16_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint32_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int32_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(int64_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint64_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(double& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(uint8_t& value, Common::StringView name) {
  return getNumber(name, value);
}

bool JsonInputValueSerializer::operator()(std::string& value, Common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }
  value = ptr->getString();
  return true;
}

bool JsonInputValueSerializer::operator()(bool& value, Common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }
  value = ptr->getBool();
  return true;
}

bool JsonInputValueSerializer::binary(void* value, size_t size, Common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }

  Common::fromHex(ptr->getString(), value, size);
  return true;
}

bool JsonInputValueSerializer::binary(std::string& value, Common::StringView name) {
  auto ptr = getValue(name);
  if (ptr == nullptr) {
    return false;
  }

  std::string valueHex = ptr->getString();
  value = Common::asString(Common::fromHex(valueHex));

  return true;
}

const JsonValue* JsonInputValueSerializer::getValue(Common::StringView name) {
  const JsonValue& val = *chain.back();
  if (val.isArray()) {
    return &val[idxs.back()++];
  }

  std::string strName(name);
  return val.contains(strName) ? &val(strName) : nullptr;
}
