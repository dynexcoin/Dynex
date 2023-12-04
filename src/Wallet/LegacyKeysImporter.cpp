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


#include "LegacyKeysImporter.h"

#include <vector>
#include <system_error>

#include "Common/StringTools.h"

#include "DynexCNCore/Currency.h"
#include "DynexCNCore/Account.h"
#include "DynexCNCore/DynexCNTools.h"

#include "Serialization/SerializationTools.h"

#include "WalletLegacy/WalletLegacySerializer.h"
#include "WalletLegacy/WalletUserTransactionsCache.h"
#include "Wallet/WalletUtils.h"
#include "Wallet/WalletErrors.h"

using namespace Crypto;

namespace {

struct keys_file_data {
  chacha8_iv iv;
  std::string account_data;

  void serialize(DynexCN::ISerializer& s) {
    s(iv, "iv");
    s(account_data, "account_data");
  }
};

void loadKeysFromFile(const std::string& filename, const std::string& password, DynexCN::AccountBase& account) {
  keys_file_data keys_file_data;
  std::string buf;

  if (!Common::loadFileToString(filename, buf)) {
    throw std::system_error(make_error_code(DynexCN::error::INTERNAL_WALLET_ERROR), "failed to load \"" + filename + '\"');
  }

  if (!DynexCN::fromBinaryArray(keys_file_data, Common::asBinaryArray(buf))) {
    throw std::system_error(make_error_code(DynexCN::error::INTERNAL_WALLET_ERROR), "failed to deserialize \"" + filename + '\"');
  }

  chacha8_key key;
  cn_context cn_context;
  generate_chacha8_key(cn_context, password, key);
  std::string account_data;
  account_data.resize(keys_file_data.account_data.size());
  chacha8(keys_file_data.account_data.data(), keys_file_data.account_data.size(), key, keys_file_data.iv, &account_data[0]);

  if (!DynexCN::loadFromBinaryKeyValue(account, account_data)) {
    throw std::system_error(make_error_code(DynexCN::error::WRONG_PASSWORD));
  }

  const DynexCN::AccountKeys& keys = account.getAccountKeys();
  DynexCN::throwIfKeysMissmatch(keys.viewSecretKey, keys.address.viewPublicKey);
  DynexCN::throwIfKeysMissmatch(keys.spendSecretKey, keys.address.spendPublicKey);
}

}

namespace DynexCN {

void importLegacyKeys(const std::string& legacyKeysFilename, const std::string& password, std::ostream& destination) {
  DynexCN::AccountBase account;

  loadKeysFromFile(legacyKeysFilename, password, account);

  DynexCN::WalletUserTransactionsCache transactionsCache;
  std::string cache;
  DynexCN::WalletLegacySerializer importer(account, transactionsCache);
  importer.serialize(destination, password, false, cache);
}

} //namespace DynexCN
