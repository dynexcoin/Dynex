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


#include "PaymentServiceJsonRpcMessages.h"
#include "Serialization/SerializationOverloads.h"

namespace PaymentService {

void Save::Request::serialize(DynexCN::ISerializer& /*serializer*/) {
}

void Save::Response::serialize(DynexCN::ISerializer& /*serializer*/) {
}

void Reset::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(viewSecretKey, "viewSecretKey");
  serializer(scanHeight, "scanHeight");
}

void Reset::Response::serialize(DynexCN::ISerializer& serializer) {
}

void Export::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(fileName, "fileName")) {
    throw RequestSerializationError();
  }
}

void Export::Response::serialize(DynexCN::ISerializer& serializer) {
}

void GetViewKey::Request::serialize(DynexCN::ISerializer& serializer) {
}

void GetViewKey::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(viewSecretKey, "viewSecretKey");
}

void GetMnemonicSeed::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void GetMnemonicSeed::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(mnemonicSeed, "mnemonicSeed");
}

void GetStatus::Request::serialize(DynexCN::ISerializer& serializer) {
}

void GetStatus::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(blockCount, "blockCount");
  serializer(knownBlockCount, "knownBlockCount");
  serializer(localDaemonBlockCount, "localDaemonBlockCount");
  serializer(lastBlockHash, "lastBlockHash");
  serializer(peerCount, "peerCount");
  serializer(minimalFee, "minimalFee");
  serializer(version, "version");
}

void ValidateAddress::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(address, "address");
}

void ValidateAddress::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(isvalid, "isvalid");
  serializer(address, "address");
  serializer(spendPublicKey, "spendPublicKey");
  serializer(viewPublicKey, "viewPublicKey");
}

void GetAddresses::Request::serialize(DynexCN::ISerializer& serializer) {
}

void GetAddresses::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");
}

void CreateAddress::Request::serialize(DynexCN::ISerializer& serializer) {
  bool hasSecretKey = serializer(spendSecretKey, "spendSecretKey");
  bool hasPublicKey = serializer(spendPublicKey, "spendPublicKey");
  bool hasScanHeight = serializer(scanHeight, "scanHeight");
  bool hasReset = serializer(reset, "reset");
  if (!hasReset && !hasScanHeight)
     reset = true;

  if (hasSecretKey && hasPublicKey) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }

  if (hasScanHeight && hasReset) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
}

void CreateAddress::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(address, "address");
}

void CreateAddressList::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(spendSecretKeys, "spendSecretKeys")) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
  bool hasReset = serializer(reset, "reset");
  if (!hasReset)
    reset = true;
  bool hasScanHeights = serializer(scanHeights, "scanHeights");
  if (hasScanHeights && hasReset) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
  if (hasScanHeights && scanHeights.size() != spendSecretKeys.size()) {
    //TODO: replace it with error codes
    throw RequestSerializationError();
  }
}

void CreateAddressList::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");
}

void DeleteAddress::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void DeleteAddress::Response::serialize(DynexCN::ISerializer& serializer) {
}

void GetSpendKeys::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void GetSpendKeys::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(spendSecretKey, "spendSecretKey");
  serializer(spendPublicKey, "spendPublicKey");
}

void GetBalance::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(address, "address");
}

void GetBalance::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(availableBalance, "availableBalance");
  serializer(lockedAmount, "lockedAmount");
}

void GetBlockHashes::Request::serialize(DynexCN::ISerializer& serializer) {
  bool r = serializer(firstBlockIndex, "firstBlockIndex");
  r &= serializer(blockCount, "blockCount");

  if (!r) {
    throw RequestSerializationError();
  }
}

void GetBlockHashes::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(blockHashes, "blockHashes");
}

void TransactionHashesInBlockRpcInfo::serialize(DynexCN::ISerializer& serializer) {
  serializer(blockHash, "blockHash");
  serializer(transactionHashes, "transactionHashes");
}

void GetTransactionHashes::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex")) {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount")) {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactionHashes::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(items, "items");
}

void TransferRpcInfo::serialize(DynexCN::ISerializer& serializer) {
  serializer(type, "type");
  serializer(address, "address");
  serializer(amount, "amount");
}

void TransactionRpcInfo::serialize(DynexCN::ISerializer& serializer) {
  serializer(state, "state");
  serializer(transactionHash, "transactionHash");
  serializer(blockIndex, "blockIndex");
  serializer(confirmations, "confirmations");
  serializer(timestamp, "timestamp");
  serializer(isBase, "isBase");
  serializer(unlockTime, "unlockTime");
  serializer(amount, "amount");
  serializer(fee, "fee");
  serializer(transfers, "transfers");
  serializer(extra, "extra");
  serializer(paymentId, "paymentId");
}

void GetTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void GetTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transaction, "transaction");
}

void TransactionsInBlockRpcInfo::serialize(DynexCN::ISerializer& serializer) {
  serializer(blockHash, "blockHash");
  serializer(transactions, "transactions");
}

void GetTransactions::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (serializer(blockHash, "blockHash") == serializer(firstBlockIndex, "firstBlockIndex")) {
    throw RequestSerializationError();
  }

  if (!serializer(blockCount, "blockCount")) {
    throw RequestSerializationError();
  }

  serializer(paymentId, "paymentId");
}

void GetTransactions::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(items, "items");
}

void GetUnconfirmedTransactionHashes::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");
}

void GetUnconfirmedTransactionHashes::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionHashes, "transactionHashes");
}

void GetTransactionSecretKey::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void GetTransactionSecretKey::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionSecretKey, "transactionSecretKey");
}

void GetTransactionProof::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
  if (!serializer(destinationAddress, "destinationAddress")) {
    throw RequestSerializationError();
  }
  serializer(transactionSecretKey, "transactionSecretKey");
}

void GetTransactionProof::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionProof, "transactionProof");
}

void GetReserveProof::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
  serializer(amount, "amount");
  serializer(message, "message");
}

void GetReserveProof::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(reserveProof, "reserveProof");
}

// offline-signature:
void ExportOutputs::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(address, "address")) {
    throw RequestSerializationError();
  }
}

void ExportOutputs::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(message, "message");
}
// --

void WalletRpcOrder::serialize(DynexCN::ISerializer& serializer) {
  bool r = serializer(address, "address");
  r &= serializer(amount, "amount");

  if (!r) {
    throw RequestSerializationError();
  }
}

void SendTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(sourceAddresses, "addresses");

  if (!serializer(transfers, "transfers")) {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee")) {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity")) {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId) {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void SendTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionHash, "transactionHash");
  serializer(transactionSecretKey, "transactionSecretKey");
}

void CreateDelayedTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  serializer(addresses, "addresses");

  if (!serializer(transfers, "transfers")) {
    throw RequestSerializationError();
  }

  serializer(changeAddress, "changeAddress");

  if (!serializer(fee, "fee")) {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity")) {
    throw RequestSerializationError();
  }

  bool hasExtra = serializer(extra, "extra");
  bool hasPaymentId = serializer(paymentId, "paymentId");

  if (hasExtra && hasPaymentId) {
    throw RequestSerializationError();
  }

  serializer(unlockTime, "unlockTime");
}

void CreateDelayedTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionHash, "transactionHash");
}

void GetDelayedTransactionHashes::Request::serialize(DynexCN::ISerializer& serializer) {
}

void GetDelayedTransactionHashes::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionHashes, "transactionHashes");
}

void DeleteDelayedTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void DeleteDelayedTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
}

void SendDelayedTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(transactionHash, "transactionHash")) {
    throw RequestSerializationError();
  }
}

void SendDelayedTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
}

void SendFusionTransaction::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(threshold, "threshold")) {
    throw RequestSerializationError();
  }

  if (!serializer(anonymity, "anonymity")) {
    throw RequestSerializationError();
  }

  serializer(addresses, "addresses");
  serializer(destinationAddress, "destinationAddress");
}

void SendFusionTransaction::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(transactionHash, "transactionHash");
}

void EstimateFusion::Request::serialize(DynexCN::ISerializer& serializer) {
  if (!serializer(threshold, "threshold")) {
    throw RequestSerializationError();
  }

  serializer(addresses, "addresses");
}

void EstimateFusion::Response::serialize(DynexCN::ISerializer& serializer) {
  serializer(fusionReadyCount, "fusionReadyCount");
  serializer(totalOutputCount, "totalOutputCount");
}

}
