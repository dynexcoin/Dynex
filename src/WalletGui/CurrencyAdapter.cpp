// Copyright (c) 2022-2023, Dynex Developers
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
// Copyright (c) 2012-2017 The CN developers
// Copyright (c) 2012-2017 The Bytecoin developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2014-2018 The Monero project
// Copyright (c) 2014-2018 The Forknote developers
// Copyright (c) 2018-2019 The TurtleCoin developers
// Copyright (c) 2016-2022 The Karbo developers

#include "CurrencyAdapter.h"
#include "version.h"
#include "LoggerAdapter.h"

namespace WalletGui {

CurrencyAdapter& CurrencyAdapter::instance() {
  static CurrencyAdapter inst;
  return inst;
}

CurrencyAdapter::CurrencyAdapter() : m_currency(DynexCN::CurrencyBuilder(LoggerAdapter::instance().getLoggerManager()).currency()) {
}

CurrencyAdapter::~CurrencyAdapter() {
}

const DynexCN::Currency& CurrencyAdapter::getCurrency() {
  return m_currency;
}

int CurrencyAdapter::getNumberOfDecimalPlaces() const {
  return (int)m_currency.numberOfDecimalPlaces();
}

QString CurrencyAdapter::getCurrencyDisplayName() const {
  return CN_CURRENCY_DISPLAY_NAME;
}

QString CurrencyAdapter::getCurrencyName() const {
  return DynexCN::CRYPTONOTE_NAME;
}

QString CurrencyAdapter::getCurrencyTicker() const {
  return CN_CURRENCY_TICKER;
}

quint64 CurrencyAdapter::getMinimumFee() const {
  return m_currency.minimumFee();
}

quint64 CurrencyAdapter::getAddressPrefix() const {
  return m_currency.publicAddressBase58Prefix();
}

QString CurrencyAdapter::formatAmount(quint64 _amount) const {
  QString result = QString::number(_amount);
  if (result.length() < getNumberOfDecimalPlaces() + 1) {
    result = result.rightJustified(getNumberOfDecimalPlaces() + 1, '0');
  }

  int dot_pos = result.length() - getNumberOfDecimalPlaces();
  for (int pos = result.length() - 1; pos > dot_pos + 1; --pos) {
    if (result[pos] == '0') {
      result.remove(pos, 1);
    } else {
      break;
    }
  }

  result.insert(dot_pos, ".");
  for (int pos = dot_pos - 3; pos > 0; pos -= 3) {
    if (result[pos - 1].isDigit()) {
      result.insert(pos, ',');
    }
  }

  return result;
}

quint64 CurrencyAdapter::parseAmount(const QString& _amountString) const {
  QString amountString = _amountString.trimmed();
  amountString.remove(',');

  int pointIndex = amountString.indexOf('.');
  int fractionSize;
  if (pointIndex != -1) {
    fractionSize = amountString.length() - pointIndex - 1;
    while (getNumberOfDecimalPlaces() < fractionSize && amountString.right(1) == "0") {
      amountString.remove(amountString.length() - 1, 1);
      --fractionSize;
    }

    if (getNumberOfDecimalPlaces() < fractionSize) {
      return 0;
    }

    amountString.remove(pointIndex, 1);
  } else {
    fractionSize = 0;
  }

  if (amountString.isEmpty()) {
    return 0;
  }

  for (int i = 0; i < getNumberOfDecimalPlaces() - fractionSize; ++i) {
    amountString.append('0');
  }

  return amountString.toULongLong();
}

bool CurrencyAdapter::validateAddress(const QString& _address) const {
  DynexCN::AccountPublicAddress internalAddress;
  return m_currency.parseAccountAddressString(_address.toStdString(), internalAddress);
}

bool CurrencyAdapter::validatePaymentId(const QString& _paymentId) {
  if (!_paymentId.isEmpty()) {
    QByteArray hex(_paymentId.toUtf8());
    QByteArray bin = QByteArray::fromHex(hex);
    if (bin.toHex() != _paymentId.toLower() || bin.size() != sizeof(Crypto::Hash)) {
	   return false;
	}
  }
  return true;
}

}
