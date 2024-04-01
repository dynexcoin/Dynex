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

#include <cstddef>
#include <cstring>
#include <functional>

#define CRYPTO_MAKE_COMPARABLE(type) \
namespace Crypto { \
  inline bool operator==(const type &_v1, const type &_v2) { \
    return std::memcmp(&_v1, &_v2, sizeof(type)) == 0; \
  } \
  inline bool operator!=(const type &_v1, const type &_v2) { \
    return std::memcmp(&_v1, &_v2, sizeof(type)) != 0; \
  } \
}

#define CRYPTO_MAKE_HASHABLE(type) \
CRYPTO_MAKE_COMPARABLE(type) \
namespace Crypto { \
  static_assert(sizeof(size_t) <= sizeof(type), "Size of " #type " must be at least that of size_t"); \
  inline size_t hash_value(const type &_v) { \
    return reinterpret_cast<const size_t &>(_v); \
  } \
} \
namespace std { \
  template<> \
  struct hash<Crypto::type> { \
    size_t operator()(const Crypto::type &_v) const { \
      return reinterpret_cast<const size_t &>(_v); \
    } \
  }; \
}
