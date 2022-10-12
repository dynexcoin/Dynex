// Copyright (c) 2021-2022, The TuringX Project
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
// Parts of this file are originally copyright (c) 2012-2016 The Cryptonote developers

#include "crypto/crypto.h" //for rand()
#include "CryptoNoteCore/Account.h"
#include "CryptoNoteCore/CryptoNoteFormatUtils.h"
#include "CryptoNoteCore/CryptoNoteTools.h"

#include "WalletLegacy/WalletTransactionSender.h"
#include "WalletLegacy/WalletUtils.h"

#include "CryptoNoteCore/CryptoNoteBasicImpl.h"

#include <Logging/LoggerGroup.h>

#include <random>

using namespace Crypto;

auto last_tx_rpc = std::chrono::high_resolution_clock::now();

namespace {

using namespace CryptoNote;

uint64_t countNeededMoney(uint64_t fee, const std::vector<WalletLegacyTransfer>& transfers) {
  uint64_t needed_money = fee;
  for (auto& transfer: transfers) {
    throwIf(transfer.amount == 0, error::ZERO_DESTINATION);
    throwIf(transfer.amount < 0, error::WRONG_AMOUNT);
    needed_money += transfer.amount;
    throwIf(static_cast<int64_t>(needed_money) < transfer.amount, error::SUM_OVERFLOW);
  }
  return needed_money;
}

void createChangeDestinations(const AccountPublicAddress& address, uint64_t neededMoney, uint64_t foundMoney, TransactionDestinationEntry& changeDts) {
  if (neededMoney < foundMoney) {
    changeDts.addr = address;
    changeDts.amount = foundMoney - neededMoney;
  }
}

void constructTx(const AccountKeys keys, const std::vector<TransactionSourceEntry>& sources, const std::vector<TransactionDestinationEntry>& splittedDests,
    const std::string& extra, uint64_t unlockTimestamp, uint64_t sizeLimit, Transaction& tx) {
  std::vector<uint8_t> extraVec;
  extraVec.reserve(extra.size());
  std::for_each(extra.begin(), extra.end(), [&extraVec] (const char el) { extraVec.push_back(el);});

  Logging::LoggerGroup nullLog;
  bool r = constructTransaction(keys, sources, splittedDests, extraVec, tx, unlockTimestamp, nullLog);

  std::cout << "Info: This transaction used " << getObjectBinarySize(tx) << " bytes ";
  std::cout << "(maximum " << sizeLimit << ")" << std::endl;

  throwIf(!r, error::INTERNAL_WALLET_ERROR);
  throwIf(getObjectBinarySize(tx) >= sizeLimit, error::TRANSACTION_SIZE_TOO_BIG);
}

std::shared_ptr<WalletLegacyEvent> makeCompleteEvent(WalletUserTransactionsCache& transactionCache, size_t transactionId, std::error_code ec) {
  transactionCache.updateTransactionSendingState(transactionId, ec);
  return std::make_shared<WalletSendTransactionCompletedEvent>(transactionId, ec);
}

} //namespace

namespace CryptoNote {

WalletTransactionSender::WalletTransactionSender(const Currency& currency, WalletUserTransactionsCache& transactionsCache, AccountKeys keys, ITransfersContainer& transfersContainer) :
  m_currency(currency),
  m_transactionsCache(transactionsCache),
  m_isStoping(false),
  m_keys(keys),
  m_transferDetails(transfersContainer),
  m_upperTransactionSizeLimit(m_currency.blockGrantedFullRewardZone() * 2 - m_currency.minerTxBlobReservedSize()) {}

void WalletTransactionSender::stop() {
  m_isStoping = true;
}

bool WalletTransactionSender::validateDestinationAddress(const std::string& address) {
  AccountPublicAddress ignore;
  return m_currency.parseAccountAddressString(address, ignore);
}

void WalletTransactionSender::validateTransfersAddresses(const std::vector<WalletLegacyTransfer>& transfers) {
  for (const WalletLegacyTransfer& tr : transfers) {
    if (!validateDestinationAddress(tr.address)) {
      throw std::system_error(make_error_code(error::BAD_ADDRESS));
    }
  }
}

std::shared_ptr<WalletRequest> WalletTransactionSender::makeSendRequest(TransactionId& transactionId, std::deque<std::shared_ptr<WalletLegacyEvent>>& events,
    const std::vector<WalletLegacyTransfer>& transfers, uint64_t fee, const std::string& extra, uint64_t mixIn, uint64_t unlockTimestamp) {

  ////// FORCE MIXIN = 0 //////////////////////////////////////////////
  mixIn = 0;
  /////////////////////////////////////////////////////////////////////

  using namespace CryptoNote;
  throwIf(transfers.empty(), error::ZERO_DESTINATION);
  validateTransfersAddresses(transfers);
  uint64_t neededMoney = countNeededMoney(fee, transfers);
  std::shared_ptr<SendTransactionContext> context = std::make_shared<SendTransactionContext>();

  //neededMoney = transfer amount + fee
  //here the random inputs are being created
  //context->foundMoney = selectTransfersToSend(neededMoney, 0 == mixIn, context->dustPolicy.dustThreshold, context->selectedTransfers);
  context->foundMoney = selectTransfersToSend(neededMoney, 0 == mixIn, context->dustPolicy.dustThreshold, context->selectedTransfers, neededMoney-fee, fee, mixIn); //dm sending original amount, fee and mixIn
  throwIf(context->foundMoney < neededMoney, error::WRONG_AMOUNT);

  transactionId = m_transactionsCache.addNewTransaction(neededMoney, fee, extra, transfers, unlockTimestamp);
  context->transactionId = transactionId;
  context->mixIn = mixIn;

  if(context->mixIn && context->mixIn>0) {
  //if(context->mixIn) { //dm
    std::shared_ptr<WalletRequest> request = makeGetRandomOutsRequest(context);
    return request;
  }

  return doSendTransaction(context, events);
}

std::shared_ptr<WalletRequest> WalletTransactionSender::makeGetRandomOutsRequest(std::shared_ptr<SendTransactionContext> context) {
  uint64_t outsCount = context->mixIn + 1;// add one to make possible (if need) to skip real output key
  std::vector<uint64_t> amounts;

  for (const auto& td : context->selectedTransfers) {
    amounts.push_back(td.amount);
  }

  return std::make_shared<WalletGetRandomOutsByAmountsRequest>(amounts, outsCount, context, std::bind(&WalletTransactionSender::sendTransactionRandomOutsByAmount,
      this, context, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void WalletTransactionSender::sendTransactionRandomOutsByAmount(std::shared_ptr<SendTransactionContext> context, std::deque<std::shared_ptr<WalletLegacyEvent>>& events,
    boost::optional<std::shared_ptr<WalletRequest> >& nextRequest, std::error_code ec) {
  
  //std::cout << "*** DEBUG sendTransactionRandomOutsByAmount *** mixIn=" << context->mixIn << std::endl;

  if (m_isStoping) {
    ec = make_error_code(error::TX_CANCELLED);
  }

  if (ec) {
    events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, ec));
    return;
  }

  auto scanty_it = std::find_if(context->outs.begin(), context->outs.end(), 
    [&] (COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& out) {return out.outs.size() < context->mixIn;});

  if (scanty_it != context->outs.end()) {
    events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, make_error_code(error::MIXIN_COUNT_TOO_BIG)));
    return;
  }

  std::shared_ptr<WalletRequest> req = doSendTransaction(context, events);
  if (req)
    nextRequest = req;
}

std::shared_ptr<WalletRequest> WalletTransactionSender::doSendTransaction(std::shared_ptr<SendTransactionContext> context, std::deque<std::shared_ptr<WalletLegacyEvent>>& events) {
  if (m_isStoping) {
    events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, make_error_code(error::TX_CANCELLED)));
    return std::shared_ptr<WalletRequest>();
  }

  try
  {
    WalletLegacyTransaction& transaction = m_transactionsCache.getTransaction(context->transactionId);

    std::cout << "*** DEBUG *** doSendTransaction " << std::endl;
    for (const auto& td: context->selectedTransfers) {
      std::cout << "   -> selectedTransfers amount=" << td.amount << std::endl;
    }

    //// force mixin:
    context->mixIn = 0;

    ////// CHECK LAST TRANSACTION TIMESTAMP /////////////////////////////
    auto current_tx = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = current_tx - last_tx_rpc; 
    double elapsed_s = elapsed.count()/1000;
    if (elapsed_s<60) {
        double send_in = 60-elapsed_s;
        std::string retmes = "Please send money in " + std::to_string(send_in) + " seconds";
        std::cout << "ERROR: Transfer time limit restriction: "<< retmes << std::endl;
        throw std::system_error(make_error_code(error::TIME_LIMIT), retmes);
    }
    // ok:
    last_tx_rpc = std::chrono::high_resolution_clock::now();
    /////////////////////////////////////////////////////////////////////

    

    std::vector<TransactionSourceEntry> sources;
    prepareInputs(context->selectedTransfers, context->outs, sources, context->mixIn);

    TransactionDestinationEntry changeDts;
    changeDts.amount = 0;
    uint64_t totalAmount = -transaction.totalAmount;
    createChangeDestinations(m_keys.address, totalAmount, context->foundMoney, changeDts);

    std::vector<TransactionDestinationEntry> splittedDests;
    splitDestinations(transaction.firstTransferId, transaction.transferCount, changeDts, context->dustPolicy, splittedDests);

    Transaction tx;
    constructTx(m_keys, sources, splittedDests, transaction.extra, transaction.unlockTime, m_upperTransactionSizeLimit, tx);

    getObjectHash(tx, transaction.hash);

    m_transactionsCache.updateTransaction(context->transactionId, tx, totalAmount, context->selectedTransfers);

    notifyBalanceChanged(events);
   
    return std::make_shared<WalletRelayTransactionRequest>(tx, std::bind(&WalletTransactionSender::relayTransactionCallback, this, context,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
  catch(std::system_error& ec) {
    events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, ec.code()));
  }
  catch(std::exception&) {
    events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, make_error_code(error::INTERNAL_WALLET_ERROR)));
  }

  return std::shared_ptr<WalletRequest>();
}

void WalletTransactionSender::relayTransactionCallback(std::shared_ptr<SendTransactionContext> context, std::deque<std::shared_ptr<WalletLegacyEvent>>& events,
                                                       boost::optional<std::shared_ptr<WalletRequest> >& nextRequest, std::error_code ec) {
  if (m_isStoping) {
    return;
  }

  events.push_back(makeCompleteEvent(m_transactionsCache, context->transactionId, ec));
}


void WalletTransactionSender::splitDestinations(TransferId firstTransferId, size_t transfersCount, const TransactionDestinationEntry& changeDts,
  const TxDustPolicy& dustPolicy, std::vector<TransactionDestinationEntry>& splittedDests) {
  uint64_t dust = 0;

  digitSplitStrategy(firstTransferId, transfersCount, changeDts, dustPolicy.dustThreshold, splittedDests, dust);

  throwIf(dustPolicy.dustThreshold < dust, error::INTERNAL_WALLET_ERROR);
  if (0 != dust && !dustPolicy.addToFee) {
    splittedDests.push_back(TransactionDestinationEntry(dust, dustPolicy.addrForDust));
  }
}


void WalletTransactionSender::digitSplitStrategy(TransferId firstTransferId, size_t transfersCount,
  const TransactionDestinationEntry& change_dst, uint64_t dust_threshold,
  std::vector<TransactionDestinationEntry>& splitted_dsts, uint64_t& dust) {
  splitted_dsts.clear();
  dust = 0;

  for (TransferId idx = firstTransferId; idx < firstTransferId + transfersCount; ++idx) {
    WalletLegacyTransfer& de = m_transactionsCache.getTransfer(idx);

    AccountPublicAddress addr;
    if (!m_currency.parseAccountAddressString(de.address, addr)) {
      throw std::system_error(make_error_code(error::BAD_ADDRESS));
    }

    ////dm: do not decompose:
    decompose_amount_into_digits(de.amount, dust_threshold,
      [&](uint64_t chunk) { splitted_dsts.push_back(TransactionDestinationEntry(chunk, addr)); },
      [&](uint64_t a_dust) { splitted_dsts.push_back(TransactionDestinationEntry(a_dust, addr)); });
  }

  decompose_amount_into_digits(change_dst.amount, dust_threshold,
    [&](uint64_t chunk) { splitted_dsts.push_back(TransactionDestinationEntry(chunk, change_dst.addr)); },
    [&](uint64_t a_dust) { dust = a_dust; } );
}


void WalletTransactionSender::prepareInputs(
  const std::list<TransactionOutputInformation>& selectedTransfers,
  std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount>& outs,
  std::vector<TransactionSourceEntry>& sources, uint64_t mixIn) {

  ///// force mixin:
  mixIn = 0;
  //std::cout << "*** DEBUG *** prepareInputs" << std::endl;

  size_t i = 0;

  for (const auto& td: selectedTransfers) {
    sources.resize(sources.size()+1);
    TransactionSourceEntry& src = sources.back();

    src.amount = td.amount;
    
    //paste mixin transaction
    if(outs.size() && mixIn>0) {
    //if(outs.size()) { //dm
      std::sort(outs[i].outs.begin(), outs[i].outs.end(),
        [](const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& a, const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry& b){return a.global_amount_index < b.global_amount_index;});
      for (auto& daemon_oe: outs[i].outs) {
        if(td.globalOutputIndex == daemon_oe.global_amount_index)
          continue;
        TransactionSourceEntry::OutputEntry oe;
        oe.first = static_cast<uint32_t>(daemon_oe.global_amount_index);
        oe.second = daemon_oe.out_key;
        src.outputs.push_back(oe);
        if(src.outputs.size() >= mixIn)
          break;
      }
    }

    //paste real transaction to the random index
    auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const TransactionSourceEntry::OutputEntry& a) { return a.first >= td.globalOutputIndex; });

    TransactionSourceEntry::OutputEntry real_oe;
    real_oe.first = td.globalOutputIndex;
    real_oe.second = td.outputKey;

    auto interted_it = src.outputs.insert(it_to_insert, real_oe);

    src.realTransactionPublicKey = td.transactionPublicKey;
    src.realOutput = interted_it - src.outputs.begin();
    src.realOutputIndexInTransaction = td.outputInTransaction;
    ++i;
  }
}

void WalletTransactionSender::notifyBalanceChanged(std::deque<std::shared_ptr<WalletLegacyEvent>>& events) {
  uint64_t unconfirmedOutsAmount = m_transactionsCache.unconfrimedOutsAmount();
  uint64_t change = unconfirmedOutsAmount - m_transactionsCache.unconfirmedTransactionsAmount();

  uint64_t actualBalance = m_transferDetails.balance(ITransfersContainer::IncludeKeyUnlocked) - unconfirmedOutsAmount;
  uint64_t pendingBalance = m_transferDetails.balance(ITransfersContainer::IncludeKeyNotUnlocked) + change;

  events.push_back(std::make_shared<WalletActualBalanceUpdatedEvent>(actualBalance));
  events.push_back(std::make_shared<WalletPendingBalanceUpdatedEvent>(pendingBalance));
}

namespace {

template<typename URNG, typename T>
T popRandomValue(URNG& randomGenerator, std::vector<T>& vec) {
  assert(!vec.empty());

  if (vec.empty()) {
    return T();
  }

  std::uniform_int_distribution<size_t> distribution(0, vec.size() - 1);
  size_t idx = distribution(randomGenerator);

  T res = vec[idx];
  if (idx + 1 != vec.size()) {
    vec[idx] = vec.back();
  }
  vec.resize(vec.size() - 1);

  return res;
}

}


//uint64_t WalletTransactionSender::selectTransfersToSend(uint64_t neededMoney, bool addDust, uint64_t dust, std::list<TransactionOutputInformation>& selectedTransfers) { //dm

uint64_t WalletTransactionSender::selectTransfersToSend(uint64_t neededMoney, bool addDust, uint64_t dust, std::list<TransactionOutputInformation>& selectedTransfers, uint64_t orig_amount, uint64_t orig_fee, uint64_t mixIn) { //dm

  // info: neededMoney = transfer amount + fee
  /*std::cout << "*** DEBUG *** selectTransfersToSend. amount=" << orig_amount << " fee=" << orig_fee << " mixIn=" << mixIn << std::endl;
  std::cout << "*** DEBUG *** selectTransfersToSend. selectedTransfers:" << std::endl;
  for (const auto& td: selectedTransfers) {
      std::cout << "   pre: -> selectedTransfers amount=" << td.amount << std::endl;
  }*/

  std::vector<size_t> unusedTransfers;
  std::vector<size_t> unusedDust;

  std::vector<TransactionOutputInformation> outputs;
  m_transferDetails.getOutputs(outputs, ITransfersContainer::IncludeKeyUnlocked);

  //dm: do we have matching outs?
  std::vector<size_t> idx_amount;
  int idx_fee    = -1;
  if (mixIn==0) {
      for (size_t i = 0; i < outputs.size(); ++i) {
          const auto& out = outputs[i];
          if (!m_transactionsCache.isUsed(out) && out.amount==orig_amount) idx_amount.push_back(i);
          if (!m_transactionsCache.isUsed(out) && out.amount==orig_fee) idx_fee = i;
          if (idx_amount.size()>0 && idx_fee!=-1) break;
      }
          // amount not found? well then we also need to split (largest(!) together to save blob size):
          if (idx_amount.size()==0) {
              
              uint64_t found = 0;
              uint64_t cnt = 0;
              while (found < orig_amount && cnt < outputs.size()) {
                    uint64_t amount_remaining = orig_amount-found;
                    uint64_t largest_avail     = 0;
                    int      largest_avail_idx = -1;
                    for (size_t i = 0; i < outputs.size(); ++i) {
                        const auto& out = outputs[i];
                        if (!m_transactionsCache.isUsed(out) && out.amount>largest_avail && out.amount<=amount_remaining) {
                              //alredy used?
                              bool used = false;
                              for (size_t x=0; x<idx_amount.size(); x++) {
                                if (idx_amount[x]==i) used = true;
                              }
                              if (!used) {
                                  largest_avail = out.amount;
                                  largest_avail_idx = i;
                              } 
                        }
                    }
                    // found?
                    if (largest_avail_idx!=-1) {
                          found = found + largest_avail;
                          idx_amount.push_back(largest_avail_idx);
                          //std::cout << "*** DEBUG *** idx="<< largest_avail_idx << " found " << largest_avail << " -> total = " << found << std::endl;
                    }
                    cnt++;
                    
              }
              if (found != orig_amount) {
                idx_amount.clear();
              }
          }
  }

  //std::cout << "*** DEBUG *** -> amount=" << orig_amount << " fee=" << orig_fee << " neededMoney=" << neededMoney << std::endl;
  //std::cout << "*** DEBUG *** -> found amount idx=" << idx_amount.size() << " fee idx=" << idx_fee << std::endl;

  // mixin=0? then we send unobfuscated amounts: +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  if (mixIn==0 && idx_amount.size()>0 && idx_fee !=-1) {
        uint64_t foundMoney = 0;
        //amount(s):
        for (uint64_t i=0; i<idx_amount.size(); i++) {
            selectedTransfers.push_back(outputs[idx_amount[i]]);
            //std::cout << "*** DEBUG: unobfuscated selectedTransfers.push_back(): " << outputs[idx_amount[i]].amount << std::endl;
            foundMoney += outputs[idx_amount[i]].amount;
        }
        //fee:
        selectedTransfers.push_back(outputs[idx_fee]);
        //std::cout << "*** DEBUG: unobfuscated selectedTransfers.push_back(): " << outputs[idx_fee].amount << std::endl;
        foundMoney += outputs[idx_fee].amount;
        return foundMoney;

  } else { //otherwise apply mixin: ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // retrieve list of all unused outputs (random amounts), from which we choose later:
        for (size_t i = 0; i < outputs.size(); ++i) {
          const auto& out = outputs[i];
          if (!m_transactionsCache.isUsed(out)) {
            if (dust < out.amount)
              unusedTransfers.push_back(i);
            else
              unusedDust.push_back(i);
          }
        }

        std::default_random_engine randomGenerator(Crypto::rand<std::default_random_engine::result_type>());
        bool selectOneDust = addDust && !unusedDust.empty();
        uint64_t foundMoney = 0;

        while (foundMoney < neededMoney && (!unusedTransfers.empty() || !unusedDust.empty())) {
          size_t idx;
          if (selectOneDust) {
            idx = popRandomValue(randomGenerator, unusedDust);
            selectOneDust = false;
          } else {
            idx = !unusedTransfers.empty() ? popRandomValue(randomGenerator, unusedTransfers) : popRandomValue(randomGenerator, unusedDust);
          }

          selectedTransfers.push_back(outputs[idx]);
          //std::cout << "*** DEBUG: selectedTransfers.push_back(): " << outputs[idx].amount << std::endl;
          foundMoney += outputs[idx].amount;
        }
        return foundMoney;
  }

  

}


} /* namespace CryptoNote */
