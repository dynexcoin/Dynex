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


#include "WalletUtils.h"

#include "CryptoNote.h"
#include "crypto/crypto.h"
#include "Wallet/WalletErrors.h"

namespace CryptoNote {

void throwIfKeysMissmatch(const Crypto::SecretKey& secretKey, const Crypto::PublicKey& expectedPublicKey, const std::string& message) {
  Crypto::PublicKey pub;
  bool r = Crypto::secret_key_to_public_key(secretKey, pub);
  if (!r || expectedPublicKey != pub) {
    throw std::system_error(make_error_code(CryptoNote::error::WRONG_PASSWORD), message);
  }
}

bool validateAddress(const std::string& address, const CryptoNote::Currency& currency) {
  CryptoNote::AccountPublicAddress ignore;
  return currency.parseAccountAddressString(address, ignore);
}

std::ostream& operator<<(std::ostream& os, CryptoNote::WalletTransactionState state) {
  switch (state) {
  case CryptoNote::WalletTransactionState::SUCCEEDED:
    os << "SUCCEEDED";
    break;
  case CryptoNote::WalletTransactionState::FAILED:
    os << "FAILED";
    break;
  case CryptoNote::WalletTransactionState::CANCELLED:
    os << "CANCELLED";
    break;
  case CryptoNote::WalletTransactionState::CREATED:
    os << "CREATED";
    break;
  case CryptoNote::WalletTransactionState::DELETED:
    os << "DELETED";
    break;
  default:
    os << "<UNKNOWN>";
  }

  return os << " (" << static_cast<int>(state) << ')';
}

std::ostream& operator<<(std::ostream& os, CryptoNote::WalletTransferType type) {
  switch (type) {
  case CryptoNote::WalletTransferType::USUAL:
    os << "USUAL";
    break;
  case CryptoNote::WalletTransferType::DONATION:
    os << "DONATION";
    break;
  case CryptoNote::WalletTransferType::CHANGE:
    os << "CHANGE";
    break;
  default:
    os << "<UNKNOWN>";
  }

  return os << " (" << static_cast<int>(type) << ')';
}

std::ostream& operator<<(std::ostream& os, CryptoNote::WalletGreen::WalletState state) {
  switch (state) {
  case CryptoNote::WalletGreen::WalletState::INITIALIZED:
    os << "INITIALIZED";
    break;
  case CryptoNote::WalletGreen::WalletState::NOT_INITIALIZED:
    os << "NOT_INITIALIZED";
    break;
  default:
    os << "<UNKNOWN>";
  }

  return os << " (" << static_cast<int>(state) << ')';
}

std::ostream& operator<<(std::ostream& os, CryptoNote::WalletGreen::WalletTrackingMode mode) {
  switch (mode) {
  case CryptoNote::WalletGreen::WalletTrackingMode::TRACKING:
    os << "TRACKING";
    break;
  case CryptoNote::WalletGreen::WalletTrackingMode::NOT_TRACKING:
    os << "NOT_TRACKING";
    break;
  case CryptoNote::WalletGreen::WalletTrackingMode::NO_ADDRESSES:
    os << "NO_ADDRESSES";
    break;
  default:
    os << "<UNKNOWN>";
  }

  return os << " (" << static_cast<int>(mode) << ')';
}

TransferListFormatter::TransferListFormatter(const CryptoNote::Currency& currency, const WalletGreen::TransfersRange& range) :
  m_currency(currency),
  m_range(range) {
}

void TransferListFormatter::print(std::ostream& os) const {
  for (auto it = m_range.first; it != m_range.second; ++it) {
    os << '\n' << std::setw(21) << m_currency.formatAmount(it->second.amount) <<
      ' ' << (it->second.address.empty() ? "<UNKNOWN>" : it->second.address) <<
      ' ' << it->second.type;
  }
}

std::ostream& operator<<(std::ostream& os, const TransferListFormatter& formatter) {
  formatter.print(os);
  return os;
}

WalletOrderListFormatter::WalletOrderListFormatter(const CryptoNote::Currency& currency, const std::vector<CryptoNote::WalletOrder>& walletOrderList) :
  m_currency(currency),
  m_walletOrderList(walletOrderList) {
}

void WalletOrderListFormatter::print(std::ostream& os) const {
  os << '{';

  if (!m_walletOrderList.empty()) {
    os << '<' << m_currency.formatAmount(m_walletOrderList.front().amount) << ", " << m_walletOrderList.front().address << '>';

    for (auto it = std::next(m_walletOrderList.begin()); it != m_walletOrderList.end(); ++it) {
      os << '<' << m_currency.formatAmount(it->amount) << ", " << it->address << '>';
    }
  }

  os << '}';
}

std::ostream& operator<<(std::ostream& os, const WalletOrderListFormatter& formatter) {
  formatter.print(os);
  return os;
}

}
