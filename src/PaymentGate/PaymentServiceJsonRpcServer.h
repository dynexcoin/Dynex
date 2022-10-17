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

#include <unordered_map>

#include "Common/JsonValue.h"
#include "JsonRpcServer/JsonRpcServer.h"
#include "PaymentServiceJsonRpcMessages.h"
#include "Serialization/JsonInputValueSerializer.h"
#include "Serialization/JsonOutputStreamSerializer.h"

namespace PaymentService {

class WalletService;

class PaymentServiceJsonRpcServer : public CryptoNote::JsonRpcServer {
public:
  PaymentServiceJsonRpcServer(System::Dispatcher& sys, System::Event& stopEvent, WalletService& service, Logging::ILogger& loggerGroup);
  PaymentServiceJsonRpcServer(const PaymentServiceJsonRpcServer&) = delete;

protected:
  virtual void processJsonRpcRequest(const Common::JsonValue& req, Common::JsonValue& resp) override;

private:
  WalletService& service;
  Logging::LoggerRef logger;

  typedef std::function<void (const Common::JsonValue& jsonRpcParams, Common::JsonValue& jsonResponse)> HandlerFunction;

  template <typename RequestType, typename ResponseType, typename RequestHandler>
  HandlerFunction jsonHandler(RequestHandler handler) {
    return [handler] (const Common::JsonValue& jsonRpcParams, Common::JsonValue& jsonResponse) mutable {
      RequestType request;
      ResponseType response;

      try {
        CryptoNote::JsonInputValueSerializer inputSerializer(const_cast<Common::JsonValue&>(jsonRpcParams));
        serialize(request, inputSerializer);
      } catch (std::exception&) {
        makeGenericErrorReponse(jsonResponse, "Invalid Request", -32600);
        return;
      }

      std::error_code ec = handler(request, response);
      if (ec) {
        makeErrorResponse(ec, jsonResponse);
        return;
      }

      CryptoNote::JsonOutputStreamSerializer outputSerializer;
      serialize(response, outputSerializer);
      fillJsonResponse(outputSerializer.getValue(), jsonResponse);
    };
  }

  std::unordered_map<std::string, HandlerFunction> handlers;

  std::error_code handleSave(const Save::Request& request, Save::Response& response);
  std::error_code handleReset(const Reset::Request& request, Reset::Response& response);
  std::error_code handleExport(const Export::Request& request, Export::Response& response);
  std::error_code handleCreateAddress(const CreateAddress::Request& request, CreateAddress::Response& response);
  std::error_code handleCreateAddressList(const CreateAddressList::Request& request, CreateAddressList::Response& response);
  std::error_code handleDeleteAddress(const DeleteAddress::Request& request, DeleteAddress::Response& response);
  std::error_code handleGetSpendKeys(const GetSpendKeys::Request& request, GetSpendKeys::Response& response);
  std::error_code handleGetBalance(const GetBalance::Request& request, GetBalance::Response& response);
  std::error_code handleGetBlockHashes(const GetBlockHashes::Request& request, GetBlockHashes::Response& response);
  std::error_code handleGetTransactionHashes(const GetTransactionHashes::Request& request, GetTransactionHashes::Response& response);
  std::error_code handleGetTransactions(const GetTransactions::Request& request, GetTransactions::Response& response);
  std::error_code handleGetUnconfirmedTransactionHashes(const GetUnconfirmedTransactionHashes::Request& request, GetUnconfirmedTransactionHashes::Response& response);
  std::error_code handleGetTransaction(const GetTransaction::Request& request, GetTransaction::Response& response);
  std::error_code handleGetTransactionSecretKey(const GetTransactionSecretKey::Request& request, GetTransactionSecretKey::Response& response);
  std::error_code handleGetTransactionProof(const GetTransactionProof::Request& request, GetTransactionProof::Response& response);
  std::error_code handleSendTransaction(const SendTransaction::Request& request, SendTransaction::Response& response);
  std::error_code handleCreateDelayedTransaction(const CreateDelayedTransaction::Request& request, CreateDelayedTransaction::Response& response);
  std::error_code handleGetDelayedTransactionHashes(const GetDelayedTransactionHashes::Request& request, GetDelayedTransactionHashes::Response& response);
  std::error_code handleDeleteDelayedTransaction(const DeleteDelayedTransaction::Request& request, DeleteDelayedTransaction::Response& response);
  std::error_code handleSendDelayedTransaction(const SendDelayedTransaction::Request& request, SendDelayedTransaction::Response& response);
  std::error_code handleGetViewKey(const GetViewKey::Request& request, GetViewKey::Response& response);
  std::error_code handleGetMnemonicSeed(const GetMnemonicSeed::Request& request, GetMnemonicSeed::Response& response);
  std::error_code handleGetStatus(const GetStatus::Request& request, GetStatus::Response& response);
  std::error_code handleGetAddresses(const GetAddresses::Request& request, GetAddresses::Response& response);
  std::error_code handleValidateAddress(const ValidateAddress::Request& request, ValidateAddress::Response& response);
  std::error_code handleGetReserveProof(const GetReserveProof::Request& request, GetReserveProof::Response& response);

  std::error_code handleSendFusionTransaction(const SendFusionTransaction::Request& request, SendFusionTransaction::Response& response);
  std::error_code handleEstimateFusion(const EstimateFusion::Request& request, EstimateFusion::Response& response);
};

}//namespace PaymentService
