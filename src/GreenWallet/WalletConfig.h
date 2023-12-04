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

#include "DynexCNConfig.h"

/* Make sure everything in here is const - or it won't compile! */
namespace WalletConfig
{
    /* The prefix your coins address starts with */
    const std::string addressPrefix = "X";

    /* Your coins 'Ticker', e.g. Monero = XMR, Bitcoin = BTC */
    const std::string ticker = "DNX";

    /* The filename to output the CSV to in save_csv */
    const std::string csvFilename = "transactions.csv";

    /* The filename to read+write the address book to - consider starting with
       a leading '.' to make it hidden under mac+linux */
    const std::string addressBookFilename = ".addressBook.json";

    /* The name of your deamon */
    const std::string daemonName = "dynexd";

    /* The name to call this wallet */
    const std::string walletName = "GreenWallet";

    /* The name of walletd, the programmatic rpc interface to a wallet */
    const std::string walletdName = "walletd";

    /* The full name of your crypto */
    const std::string coinName = "Dynex";

    /* Where can your users contact you for support? E.g. discord */
    const std::string contactLink = "https://dynexcoin.org";

    /* The number of decimals your coin has */
    const int numDecimalPlaces = DynexCN::parameters
                                           ::CRYPTONOTE_DISPLAY_DECIMAL_POINT;


    /* The length of a standard address for your coin */
    const long unsigned int addressLength = 97;


    /* The mixin value to use with transactions */
    const uint64_t defaultMixin = 2;

    /* The default fee value to use with transactions (in ATOMIC units!) */
    const uint64_t defaultFee = DynexCN::parameters::MINIMUM_FEE; 

    /* The minimum fee value to allow with transactions (in ATOMIC units!) */
    const uint64_t minimumFee = DynexCN::parameters::MINIMUM_FEE;

    /* The minimum amount allowed to be sent - usually 1 (in ATOMIC units!) */
    const uint64_t minimumSend = 1;

    /* Is a mixin of zero disabled on your network? */
    const bool mixinZeroDisabled = false;

    /* If a mixin of zero is disabled, at what height was it disabled? E.g.
       fork height, or 0, if never allowed. This is ignored if a mixin of
       zero is allowed */
    const uint32_t mixinZeroDisabledHeight = 0;
}
