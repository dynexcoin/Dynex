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

#pragma once

#include "ISerializer.h"

#include <array>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <set>
#include <type_traits>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace CryptoNote {

template<typename T>
typename std::enable_if<std::is_pod<T>::value>::type
serializeAsBinary(std::vector<T>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  std::string blob;
  if (serializer.type() == ISerializer::INPUT) {
    serializer.binary(blob, name);
    value.resize(blob.size() / sizeof(T));
    if (blob.size()) {
      memcpy(&value[0], blob.data(), blob.size());
    }
  } else {
    if (!value.empty()) {
      blob.assign(reinterpret_cast<const char*>(&value[0]), value.size() * sizeof(T));
    }
    serializer.binary(blob, name);
  }
}

template<typename T>
typename std::enable_if<std::is_pod<T>::value>::type
serializeAsBinary(std::list<T>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  std::string blob;
  if (serializer.type() == ISerializer::INPUT) {
    serializer.binary(blob, name);

    size_t count = blob.size() / sizeof(T);
    const T* ptr = reinterpret_cast<const T*>(blob.data());

    while (count--) {
      value.push_back(*ptr++);
    }
  } else {
    if (!value.empty()) {
      blob.resize(value.size() * sizeof(T));
      T* ptr = reinterpret_cast<T*>(&blob[0]);

      for (const auto& item : value) {
        *ptr++ = item;
      }
    }
    serializer.binary(blob, name);
  }
}

template <typename Cont>
bool serializeContainer(Cont& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  size_t size = value.size();
  if (!serializer.beginArray(size, name)) {
    value.clear();
    return false;
  }

  value.resize(size);

  for (auto& item : value) {
    serializer(const_cast<typename Cont::value_type&>(item), "");
  }

  serializer.endArray();
  return true;
}

template <typename E>
bool serializeEnumClass(E& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  static_assert(std::is_enum<E>::value, "E must be an enum class");

  typedef typename std::underlying_type<E>::type EType;

  if (serializer.type() == CryptoNote::ISerializer::INPUT) {
    EType numericValue;
    serializer(numericValue, name);
    value = static_cast<E>(numericValue);
  } else {
    auto numericValue = static_cast<EType>(value);
    serializer(numericValue, name);
  }

  return true;
}

template<typename T>
bool serialize(std::vector<T>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeContainer(value, name, serializer);
}

template<typename T>
bool serialize(std::list<T>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeContainer(value, name, serializer);
}

template<typename MapT, typename ReserveOp>
bool serializeMap(MapT& value, Common::StringView name, CryptoNote::ISerializer& serializer, ReserveOp reserve) {
  size_t size = value.size();

  if (!serializer.beginArray(size, name)) {
    value.clear();
    return false;
  }

  if (serializer.type() == CryptoNote::ISerializer::INPUT) {
    reserve(size);

    for (size_t i = 0; i < size; ++i) {
      typename MapT::key_type key;
      typename MapT::mapped_type v;

      serializer.beginObject("");
      serializer(key, "key");
      serializer(v, "value");
      serializer.endObject();

      value.insert(std::make_pair(std::move(key), std::move(v)));
    }
  } else {
    for (auto& kv : value) {
      serializer.beginObject("");
      serializer(const_cast<typename MapT::key_type&>(kv.first), "key");
      serializer(kv.second, "value");
      serializer.endObject();
    }
  }

  serializer.endArray();
  return true;
}

template<typename SetT>
bool serializeSet(SetT& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  size_t size = value.size();

  if (!serializer.beginArray(size, name)) {
    value.clear();
    return false;
  }

  if (serializer.type() == CryptoNote::ISerializer::INPUT) {
    for (size_t i = 0; i < size; ++i) {
      typename SetT::value_type key;
      serializer(key, "");
      value.insert(std::move(key));
    }
  } else {
    for (auto& key : value) {
      serializer(const_cast<typename SetT::value_type&>(key), "");
    }
  }

  serializer.endArray();
  return true;
}

template<typename K, typename Hash>
bool serialize(std::unordered_set<K, Hash>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeSet(value, name, serializer);
}

template<typename K, typename Cmp>
bool serialize(std::set<K, Cmp>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeSet(value, name, serializer);
}

template<typename K, typename V, typename Hash>
bool serialize(std::unordered_map<K, V, Hash>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeMap(value, name, serializer, [&value](size_t size) { value.reserve(size); });
}

template<typename K, typename V, typename Hash>
bool serialize(std::unordered_multimap<K, V, Hash>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeMap(value, name, serializer, [&value](size_t size) { value.reserve(size); });
}

template<typename K, typename V, typename Hash>
bool serialize(std::map<K, V, Hash>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeMap(value, name, serializer, [](size_t size) {});
}

template<typename K, typename V, typename Hash>
bool serialize(std::multimap<K, V, Hash>& value, Common::StringView name, CryptoNote::ISerializer& serializer) {
  return serializeMap(value, name, serializer, [](size_t size) {});
}

template<size_t size>
bool serialize(std::array<uint8_t, size>& value, Common::StringView name, CryptoNote::ISerializer& s) {
  return s.binary(value.data(), value.size(), name);
}

template <typename T1, typename T2> 
void serialize(std::pair<T1, T2>& value, ISerializer& s) {
  s(value.first, "first");
  s(value.second, "second");
}

template <typename Element, typename Iterator>
void writeSequence(Iterator begin, Iterator end, Common::StringView name, ISerializer& s) {
  size_t size = std::distance(begin, end);
  s.beginArray(size, name);
  for (Iterator i = begin; i != end; ++i) {
    s(const_cast<Element&>(*i), "");
  }
  s.endArray();
}

template <typename Element, typename Iterator>
void readSequence(Iterator outputIterator, Common::StringView name, ISerializer& s) {
  size_t size = 0;
  s.beginArray(size, name);

  while (size--) {
    Element e;
    s(e, "");
    *outputIterator++ = std::move(e);
  }

  s.endArray();
}

//convinience function since we change block height type
void serializeBlockHeight(ISerializer& s, uint32_t& blockHeight, Common::StringView name);

//convinience function since we change global output index type
void serializeGlobalOutputIndex(ISerializer& s, uint32_t& globalOutputIndex, Common::StringView name);

}
