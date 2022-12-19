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

#include <boost/functional/hash.hpp>
#include <map>
#include <string>
#include <unordered_map>

#include "crypto/hash.h"
#include "CryptoNoteBasic.h"

namespace CryptoNote {

class ISerializer;

inline size_t paymentIdHash(const Crypto::Hash& paymentId) {
  return boost::hash_range(std::begin(paymentId.data), std::end(paymentId.data));
}

class PaymentIdIndex {
public:
  PaymentIdIndex(bool enabled);

  bool add(const Transaction& transaction);
  bool remove(const Transaction& transaction);
  bool find(const Crypto::Hash& paymentId, std::vector<Crypto::Hash>& transactionHashes);
  std::vector<Crypto::Hash> find(const Crypto::Hash& paymentId);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::unordered_multimap<Crypto::Hash, Crypto::Hash, std::function<decltype(paymentIdHash)>> index;
  bool enabled = false;
};

class TimestampBlocksIndex {
public:
  TimestampBlocksIndex(bool enabled);

  bool add(uint64_t timestamp, const Crypto::Hash& hash);
  bool remove(uint64_t timestamp, const Crypto::Hash& hash);
  bool find(uint64_t timestampBegin, uint64_t timestampEnd, uint32_t hashesNumberLimit, std::vector<Crypto::Hash>& hashes, uint32_t& hashesNumberWithinTimestamps);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::multimap<uint64_t, Crypto::Hash> index;
  bool enabled = false;
};

class TimestampTransactionsIndex {
public:
  TimestampTransactionsIndex(bool enabled);

  bool add(uint64_t timestamp, const Crypto::Hash& hash);
  bool remove(uint64_t timestamp, const Crypto::Hash& hash);
  bool find(uint64_t timestampBegin, uint64_t timestampEnd, uint64_t hashesNumberLimit, std::vector<Crypto::Hash>& hashes, uint64_t& hashesNumberWithinTimestamps);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive>
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
  }
private:
  std::multimap<uint64_t, Crypto::Hash> index;
  bool enabled = false;
};

class GeneratedTransactionsIndex {
public:
  GeneratedTransactionsIndex(bool enabled);

  bool add(const Block& block);
  bool remove(const Block& block);
  bool find(uint32_t height, uint64_t& generatedTransactions);
  void clear();

  void serialize(ISerializer& s);

  template<class Archive> 
  void serialize(Archive& archive, unsigned int version) {
    archive & index;
    archive & lastGeneratedTxNumber;
  }
private:
  std::unordered_map<uint32_t, uint64_t> index;
  uint64_t lastGeneratedTxNumber;
  bool enabled = false;
};

class OrphanBlocksIndex {
public:
  OrphanBlocksIndex(bool enabled);

  bool add(const Block& block);
  bool remove(const Block& block);
  bool find(uint32_t height, std::vector<Crypto::Hash>& blockHashes);
  void clear();
private:
  std::unordered_multimap<uint32_t, Crypto::Hash> index;
  bool enabled = false;
};

}
