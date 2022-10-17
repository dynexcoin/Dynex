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

#include <memory>

#include <GreenWallet/Types.h>
#include <GreenWallet/WalletConfig.h>

enum BalanceInfo { NotEnoughBalance, EnoughBalance, SetMixinToZero };
void transfer(std::shared_ptr<WalletInfo> walletInfo, uint32_t height,
	bool sendAll = false, std::string nodeAddress = std::string());

void doTransfer(std::string address, uint64_t amount, uint64_t fee,
                std::string extra, std::shared_ptr<WalletInfo> walletInfo,
                uint32_t height, uint64_t mixin = WalletConfig::defaultMixin,
                std::string nodeAddress = std::string(), uint64_t nodeFee = 0);

void sendMultipleTransactions(CryptoNote::WalletGreen &wallet,
                              std::vector<CryptoNote::TransactionParameters>
                              transfers);

void splitTx(CryptoNote::WalletGreen &wallet,
             CryptoNote::TransactionParameters p);

bool confirmTransaction(CryptoNote::TransactionParameters t,
                        std::shared_ptr<WalletInfo> walletInfo, uint64_t nodeFee);

bool parseAmount(std::string strAmount, uint64_t &amount);

bool parseAmount(std::string amountString);

bool parseAddress(std::string address);

bool parseFee(std::string feeString);

#ifndef __ANDROID__
bool getOpenAlias(const std::string& alias, std::string& address);

bool processServerAliasResponse(const std::string& s, std::string& address);

bool askAliasesTransfersConfirmation(const std::string address);
#endif

std::string getExtraFromPaymentID(std::string paymentID);

#ifndef __ANDROID__
std::string resolveAlias(const std::string& aliasUrl);
#endif

Maybe<std::string> getPaymentID(std::string msg);

Maybe<std::string> getExtra();

Maybe<std::string> getDestinationAddress();

Maybe<uint64_t> getFee();

Maybe<uint64_t> getTransferAmount();

BalanceInfo doWeHaveEnoughBalance(uint64_t amount, uint64_t fee,
	std::shared_ptr<WalletInfo> walletInfo,
	uint32_t height, uint64_t nodeFee);

uint64_t calculateNodeFee(uint64_t amount);
