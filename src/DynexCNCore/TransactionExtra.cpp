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


#include "TransactionExtra.h"
#include "Common/MemoryInputStream.h"
#include "Common/StreamTools.h"
#include "Common/StringTools.h"
#include "DynexCNTools.h"
#include <iostream>
#include <iomanip>
#include "Serialization/BinaryOutputStreamSerializer.h"
#include "Serialization/BinaryInputStreamSerializer.h"
#include "DynexCNCore/DynexCNBasicImpl.h"

using namespace Crypto;
using namespace Common;

namespace DynexCN {

bool parseTransactionExtra(const uint8_t* _data, size_t _size, std::vector<TransactionExtraField> &transactionExtraFields) {
  try {
    MemoryInputStream iss(_data, _size);
    BinaryInputStreamSerializer ar(iss);

    int c = 0;

    while (!iss.endOfStream()) {
      c = read<uint8_t>(iss);
      switch (c) {
      case TX_EXTRA_TAG_PADDING: {
        size_t size = 1;
        for (; !iss.endOfStream() && size <= TX_EXTRA_PADDING_MAX_COUNT; ++size) {
          if (read<uint8_t>(iss) != 0) {
            return false; // all bytes should be zero
          }
        }

        if (size > TX_EXTRA_PADDING_MAX_COUNT) {
          return false;
        }

        transactionExtraFields.push_back(TransactionExtraPadding{ size });
        break;
      }

      case TX_EXTRA_TAG_PUBKEY: {
        TransactionExtraPublicKey extraPk;
        ar(extraPk.publicKey, "public_key");
        transactionExtraFields.push_back(extraPk);
        break;
      }

      case TX_EXTRA_NONCE: {
        TransactionExtraNonce extraNonce;
        uint8_t size = read<uint8_t>(iss);

        if (size + iss.getPosition() > _size) {
          return false;
        }

        if (size > 0) {
          extraNonce.nonce.resize(size);
          read(iss, extraNonce.nonce.data(), extraNonce.nonce.size());
        }

        transactionExtraFields.push_back(extraNonce);
        break;
      }

      case TX_EXTRA_MERGE_MINING_TAG: {
        TransactionExtraMergeTag mmTag;
        ar(mmTag, "mm_tag");
        transactionExtraFields.push_back(mmTag);
        break;
      }

      case TX_EXTRA_FROM_ADDRESS: {
        TransactionExtraFromAddress address;
        read(iss, address.address.spendPublicKey.data, 32);
        read(iss, address.address.viewPublicKey.data, 32);
        transactionExtraFields.push_back(address);
        break;
      }

      case TX_EXTRA_TO_ADDRESS: {
        TransactionExtraToAddress address;
        read(iss, address.address.spendPublicKey.data, 32);
        read(iss, address.address.viewPublicKey.data, 32);
        transactionExtraFields.push_back(address);
        break;
      }

      case TX_EXTRA_AMOUNT: {
        TransactionExtraAmount amount;
        amount.amount.resize(8);
        read(iss, amount.amount.data(), 8);
        transactionExtraFields.push_back(amount);
        break;
      }

      case TX_EXTRA_TXKEY: {
        TransactionExtraTxkey tx_key;
        read(iss, tx_key.tx_key.data, 32);
        transactionExtraFields.push_back(tx_key);
        break;
      }
    }
    }
  } catch (std::exception &) {
    return false;
  }

  return true;
}

bool parseTransactionExtra(const std::vector<uint8_t> &transactionExtra, std::vector<TransactionExtraField> &transactionExtraFields) {
  transactionExtraFields.clear();
  if (transactionExtra.empty())
    return true;
  return parseTransactionExtra(transactionExtra.data(), transactionExtra.size(), transactionExtraFields);
}

bool parseTransactionExtra(const std::string &transactionExtra, std::vector<TransactionExtraField> &transactionExtraFields) {
  transactionExtraFields.clear();
  if (transactionExtra.empty())
    return true;
  return parseTransactionExtra(reinterpret_cast<const uint8_t*>(transactionExtra.data()), transactionExtra.size(), transactionExtraFields);
}

struct ExtraSerializerVisitor : public boost::static_visitor<bool> {
  std::vector<uint8_t>& extra;

  ExtraSerializerVisitor(std::vector<uint8_t>& tx_extra)
    : extra(tx_extra) {}

  bool operator()(const TransactionExtraPadding& t) {
    if (t.size > TX_EXTRA_PADDING_MAX_COUNT) {
      return false;
    }
    extra.insert(extra.end(), t.size, 0);
    return true;
  }

  bool operator()(const TransactionExtraPublicKey& t) {
    return addTransactionPublicKeyToExtra(extra, t.publicKey);
  }

  bool operator()(const TransactionExtraNonce& t) {
    return addExtraNonceToTransactionExtra(extra, t.nonce);
  }

  bool operator()(const TransactionExtraMergeTag& t) {
    return appendMergeMiningTagToExtra(extra, t);
  }

  bool operator()(const TransactionExtraFromAddress& t) {
    return addFromAddressToExtra(extra, t);
  }

  bool operator()(const TransactionExtraToAddress& t) {
    return addToAddressToExtra(extra, t);
  }

  bool operator()(const TransactionExtraAmount& t) {
    return addAmountToExtra(extra, t.amount);
  }

  bool operator()(const TransactionExtraTxkey& t) {
    return addTxkeyToExtra(extra, t.tx_key);
  }

};

bool writeTransactionExtra(std::vector<uint8_t>& tx_extra, const std::vector<TransactionExtraField>& tx_extra_fields) {
  ExtraSerializerVisitor visitor(tx_extra);

  for (const auto& tag : tx_extra_fields) {
    if (!boost::apply_visitor(visitor, tag)) {
      return false;
    }
  }

  return true;
}

PublicKey getTransactionPublicKeyFromExtra(const std::vector<uint8_t>& tx_extra) {
  std::vector<TransactionExtraField> tx_extra_fields;
  parseTransactionExtra(tx_extra, tx_extra_fields);

  TransactionExtraPublicKey pub_key_field;
  if (!findTransactionExtraFieldByType(tx_extra_fields, pub_key_field))
    return boost::value_initialized<PublicKey>();

  return pub_key_field.publicKey;
}

bool addTransactionPublicKeyToExtra(std::vector<uint8_t>& tx_extra, const PublicKey& tx_pub_key) {
  tx_extra.resize(tx_extra.size() + 1 + sizeof(PublicKey));
  tx_extra[tx_extra.size() - 1 - sizeof(PublicKey)] = TX_EXTRA_TAG_PUBKEY;
  *reinterpret_cast<PublicKey*>(&tx_extra[tx_extra.size() - sizeof(PublicKey)]) = tx_pub_key;
  return true;
}

bool addFromAddressToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraFromAddress& address) {
  size_t start_pos = tx_extra.size();
  tx_extra.resize(tx_extra.size() + 1 + 64);
  // write tag
  tx_extra[start_pos] = TX_EXTRA_FROM_ADDRESS;
  ++start_pos;
  // write address
  memcpy(&tx_extra[start_pos], address.address.spendPublicKey.data, 32);
  memcpy(&tx_extra[start_pos+32], address.address.viewPublicKey.data, 32);
  return true;
}

bool addTxkeyToExtra(std::vector<uint8_t>& tx_extra, const Crypto::SecretKey& tx_key) {
  size_t start_pos = tx_extra.size();
  tx_extra.resize(tx_extra.size() + 1 + 32);
  // write tag
  tx_extra[start_pos] = TX_EXTRA_TXKEY;
  ++start_pos;
  // write tx_key
  memcpy(&tx_extra[start_pos], tx_key.data, 32);
  return true;
}

bool addTxkeyToExtraString(const Crypto::SecretKey& tx_key, std::string& extraString) {
  extraString.append(sizeof(char), TX_EXTRA_TXKEY);
  for (int i=0; i<32; i++)
    extraString.append(sizeof(char), tx_key.data[i]);
  return true;
}

bool addFromAddressToExtraString(const AccountPublicAddress& acc, std::string& extraString) {
  extraString.append(sizeof(char), TX_EXTRA_FROM_ADDRESS);  
  for (int i=0; i<32; i++)
    extraString.append(sizeof(char), acc.spendPublicKey.data[i]);  
  for (int i=0; i<32; i++)
    extraString.append(sizeof(char), acc.viewPublicKey.data[i]);  
  return true;
}

bool addToAddressToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraToAddress& address) {
  size_t start_pos = tx_extra.size();
  tx_extra.resize(tx_extra.size() + 1 + 64);
  // write tag
  tx_extra[start_pos] = TX_EXTRA_TO_ADDRESS;
  ++start_pos;
  // write address
  memcpy(&tx_extra[start_pos], address.address.spendPublicKey.data, 32);
  memcpy(&tx_extra[start_pos+32], address.address.viewPublicKey.data, 32);
  return true;
}

std::string getAccountAddressAsStr(const AccountPublicAddress& address) {
  uint64_t prefix = 0xb9;
  return getAccountAddressAsStr(prefix, address);
}

AccountPublicAddress getAccountAddressAsKeys(const std::string& address_str) {
  AccountPublicAddress acc = boost::value_initialized<AccountPublicAddress>();
  uint64_t prefix;
  parseAccountAddressString(prefix, acc, address_str);
  return acc;
}

int64_t getAmountInt64(const std::vector<uint8_t>& amount) {
  return
        (int64_t(amount[7]) << 8*7) |
        (int64_t(amount[6]) << 8*6) |
        (int64_t(amount[5]) << 8*5) |
        (int64_t(amount[4]) << 8*4) |
        (int64_t(amount[3]) << 8*3) |
        (int64_t(amount[2]) << 8*2) |
        (int64_t(amount[1]) << 8*1) |
        (int64_t(amount[0]) << 8*0);
}

BinaryArray getBinaryAmount(int64_t amount) {
  BinaryArray amt(8);
  amt[7] = uint8_t(amount >> 8*7);
  amt[6] = uint8_t(amount >> 8*6);
  amt[5] = uint8_t(amount >> 8*5);
  amt[4] = uint8_t(amount >> 8*4);
  amt[3] = uint8_t(amount >> 8*3);
  amt[2] = uint8_t(amount >> 8*2);
  amt[1] = uint8_t(amount >> 8*1);
  amt[0] = uint8_t(amount >> 8*0);
  return amt;
}

int64_t getStringAmountInt64(const std::string& amount) {
  int64_t amt = std::stol(amount, nullptr, 16);
  return amt;
}

bool addToAddressAmountToExtraString(const AccountPublicAddress& acc, const int64_t amount, std::string& extraString) {
  extraString.append(sizeof(char), TX_EXTRA_TO_ADDRESS);  
  for (int i=0; i<32; i++)
    extraString.append(sizeof(char), acc.spendPublicKey.data[i]);  
  for (int i=0; i<32; i++)
    extraString.append(sizeof(char), acc.viewPublicKey.data[i]);  

  BinaryArray amt(getBinaryAmount(amount));
  extraString.append(sizeof(char), TX_EXTRA_AMOUNT);
  for (int i=0; i<8; i++)
    extraString.append(sizeof(char), amt[i]);  

  return true;
}

bool addAmountToExtra(std::vector<uint8_t>& tx_extra, const BinaryArray& amount) {
  if (amount.size() != 8) {
    return false;
  }
  size_t start_pos = tx_extra.size();
  tx_extra.resize(tx_extra.size() + 1 + 8);
  // write tag
  tx_extra[start_pos] = TX_EXTRA_AMOUNT;
  ++start_pos;
  // write address
  memcpy(&tx_extra[start_pos], amount.data(), 8);
  return true;
}

bool addExtraNonceToTransactionExtra(std::vector<uint8_t>& tx_extra, const BinaryArray& extra_nonce) {
  if (extra_nonce.size() > TX_EXTRA_NONCE_MAX_COUNT) {
    return false;
  }

  size_t start_pos = tx_extra.size();
  tx_extra.resize(tx_extra.size() + 2 + extra_nonce.size());
  //write tag
  tx_extra[start_pos] = TX_EXTRA_NONCE;
  //write len
  ++start_pos;
  tx_extra[start_pos] = static_cast<uint8_t>(extra_nonce.size());
  //write data
  ++start_pos;
  memcpy(&tx_extra[start_pos], extra_nonce.data(), extra_nonce.size());
  return true;
}

bool appendMergeMiningTagToExtra(std::vector<uint8_t>& tx_extra, const TransactionExtraMergeTag& mm_tag) {
  BinaryArray blob;
  if (!toBinaryArray(mm_tag, blob)) {
    return false;
  }

  tx_extra.push_back(TX_EXTRA_MERGE_MINING_TAG);
  std::copy(reinterpret_cast<const uint8_t*>(blob.data()), reinterpret_cast<const uint8_t*>(blob.data() + blob.size()), std::back_inserter(tx_extra));
  return true;
}

bool getMergeMiningTagFromExtra(const std::vector<uint8_t>& tx_extra, TransactionExtraMergeTag& mm_tag) {
  std::vector<TransactionExtraField> tx_extra_fields;
  parseTransactionExtra(tx_extra, tx_extra_fields);

  return findTransactionExtraFieldByType(tx_extra_fields, mm_tag);
}

void setPaymentIdToTransactionExtraNonce(std::vector<uint8_t>& extra_nonce, const Hash& payment_id) {
  extra_nonce.clear();
  extra_nonce.push_back(TX_EXTRA_NONCE_PAYMENT_ID);
  const uint8_t* payment_id_ptr = reinterpret_cast<const uint8_t*>(&payment_id);
  std::copy(payment_id_ptr, payment_id_ptr + sizeof(payment_id), std::back_inserter(extra_nonce));
}

bool getPaymentIdFromTransactionExtraNonce(const std::vector<uint8_t>& extra_nonce, Hash& payment_id) {
  if (sizeof(Hash) + 1 != extra_nonce.size())
    return false;
  if (TX_EXTRA_NONCE_PAYMENT_ID != extra_nonce[0])
    return false;
  payment_id = *reinterpret_cast<const Hash*>(extra_nonce.data() + 1);
  return true;
}

bool parsePaymentId(const std::string& paymentIdString, Hash& paymentId) {
  return Common::podFromHex(paymentIdString, paymentId);
}

bool createTxExtraWithPaymentId(const std::string& paymentIdString, std::vector<uint8_t>& extra) {
  Hash paymentIdBin;

  if (!parsePaymentId(paymentIdString, paymentIdBin)) {
    return false;
  }

  std::vector<uint8_t> extraNonce;
  DynexCN::setPaymentIdToTransactionExtraNonce(extraNonce, paymentIdBin);

  if (!DynexCN::addExtraNonceToTransactionExtra(extra, extraNonce)) {
    return false;
  }

  return true;
}

bool getPaymentIdFromTxExtra(const std::vector<uint8_t>& extra, Hash& paymentId) {
  std::vector<TransactionExtraField> tx_extra_fields;
  if (!parseTransactionExtra(extra, tx_extra_fields)) {
    return false;
  }

  TransactionExtraNonce extra_nonce;
  if (findTransactionExtraFieldByType(tx_extra_fields, extra_nonce)) {
    if (!getPaymentIdFromTransactionExtraNonce(extra_nonce.nonce, paymentId)) {
      return false;
    }
  } else {
    return false;
  }

  return true;
}


}
