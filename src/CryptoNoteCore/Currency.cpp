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


#include "Currency.h"
#include <cctype>
#include <boost/algorithm/string/trim.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/lexical_cast.hpp>
#include "../Common/Base58.h"
#include "../Common/int-util.h"
#include "../Common/StringTools.h"

#include "Account.h"
#include "CryptoNoteBasicImpl.h"
#include "CryptoNoteFormatUtils.h"
#include "CryptoNoteTools.h"
#include "TransactionExtra.h"
#include "UpgradeDetector.h"

#undef ERROR

using namespace Logging;
using namespace Common;

namespace CryptoNote {

	const std::vector<uint64_t> Currency::PRETTY_AMOUNTS = {
		1, 2, 3, 4, 5, 6, 7, 8, 9,
		10, 20, 30, 40, 50, 60, 70, 80, 90,
		100, 200, 300, 400, 500, 600, 700, 800, 900,
		1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
		10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
		100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
		1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
		10000000, 20000000, 30000000, 40000000, 50000000, 60000000, 70000000, 80000000, 90000000,
		100000000, 200000000, 300000000, 400000000, 500000000, 600000000, 700000000, 800000000, 900000000,
		1000000000, 2000000000, 3000000000, 4000000000, 5000000000, 6000000000, 7000000000, 8000000000, 9000000000,
		10000000000, 20000000000, 30000000000, 40000000000, 50000000000, 60000000000, 70000000000, 80000000000, 90000000000,
		100000000000, 200000000000, 300000000000, 400000000000, 500000000000, 600000000000, 700000000000, 800000000000, 900000000000,
		1000000000000, 2000000000000, 3000000000000, 4000000000000, 5000000000000, 6000000000000, 7000000000000, 8000000000000, 9000000000000,
		10000000000000, 20000000000000, 30000000000000, 40000000000000, 50000000000000, 60000000000000, 70000000000000, 80000000000000, 90000000000000,
		100000000000000, 200000000000000, 300000000000000, 400000000000000, 500000000000000, 600000000000000, 700000000000000, 800000000000000, 900000000000000,
		1000000000000000, 2000000000000000, 3000000000000000, 4000000000000000, 5000000000000000, 6000000000000000, 7000000000000000, 8000000000000000, 9000000000000000,
		10000000000000000, 20000000000000000, 30000000000000000, 40000000000000000, 50000000000000000, 60000000000000000, 70000000000000000, 80000000000000000, 90000000000000000,
		100000000000000000, 200000000000000000, 300000000000000000, 400000000000000000, 500000000000000000, 600000000000000000, 700000000000000000, 800000000000000000, 900000000000000000,
		1000000000000000000, 2000000000000000000, 3000000000000000000, 4000000000000000000, 5000000000000000000, 6000000000000000000, 7000000000000000000, 8000000000000000000, 9000000000000000000,
		10000000000000000000ull
	};

	bool Currency::init() {
		if (!generateGenesisBlock()) {
			logger(ERROR, BRIGHT_RED) << "Failed to generate genesis block";
			return false;
		}

		if (!get_block_hash(m_genesisBlock, m_genesisBlockHash)) {
			logger(ERROR, BRIGHT_RED) << "Failed to get genesis block hash";
			return false;
		}

		if (isTestnet()) {
			m_upgradeHeightV2 = 1;
			m_upgradeHeightV3 = 2;
			m_upgradeHeightV4 = 5;
			m_blocksFileName = "testnet_" + m_blocksFileName;
			m_blocksCacheFileName = "testnet_" + m_blocksCacheFileName;
			m_blockIndexesFileName = "testnet_" + m_blockIndexesFileName;
			m_txPoolFileName = "testnet_" + m_txPoolFileName;
			m_blockchainIndicesFileName = "testnet_" + m_blockchainIndicesFileName;
		}
		return true;
	}

	bool Currency::generateGenesisBlock() {
		m_genesisBlock = boost::value_initialized<Block>();

		// Hard code coinbase tx in genesis block, because "tru" generating tx use random, but genesis should be always the same
		std::string genesisCoinbaseTxHex = GENESIS_COINBASE_TX_HEX;
		BinaryArray minerTxBlob;

		bool r =
			fromHex(genesisCoinbaseTxHex, minerTxBlob) &&
			fromBinaryArray(m_genesisBlock.baseTransaction, minerTxBlob);

		if (!r) {
			logger(ERROR, BRIGHT_RED) << "failed to parse coinbase tx from hard coded blob";
			return false;
		}

		m_genesisBlock.majorVersion = BLOCK_MAJOR_VERSION_1;
		m_genesisBlock.minorVersion = BLOCK_MINOR_VERSION_0;
		m_genesisBlock.timestamp = 0;
		m_genesisBlock.nonce = 70;
		if (m_testnet) {
			++m_genesisBlock.nonce;
		}
		//miner::find_nonce_for_given_block(bl, 1, 0);

		return true;
	}

	size_t Currency::blockGrantedFullRewardZoneByBlockVersion(uint8_t blockMajorVersion) const {
		if (blockMajorVersion >= BLOCK_MAJOR_VERSION_3) {
			return m_blockGrantedFullRewardZone;
		}
		else if (blockMajorVersion == BLOCK_MAJOR_VERSION_2) {
			return CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2;
		}
		else {
			return CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1;
		}
	}

	uint32_t Currency::upgradeHeight(uint8_t majorVersion) const {
		if (majorVersion == BLOCK_MAJOR_VERSION_4) {
			return m_upgradeHeightV4;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_2) {
			return m_upgradeHeightV2;
		}
		else if (majorVersion == BLOCK_MAJOR_VERSION_3) {
			return m_upgradeHeightV3;
		}
		else {
			return static_cast<uint32_t>(-1);
		}
	}

	bool Currency::getBlockReward(uint32_t blockheight, uint8_t blockMajorVersion, size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins,
		uint64_t fee, uint64_t& reward, int64_t& emissionChange) const {
		assert(alreadyGeneratedCoins <= m_moneySupply);
		assert(m_emissionSpeedFactor > 0 && m_emissionSpeedFactor <= 8 * sizeof(uint64_t));

		// Tail emission
		uint64_t baseReward = (m_moneySupply - alreadyGeneratedCoins) >> m_emissionSpeedFactor;
        if (alreadyGeneratedCoins == 0 && m_genesisBlockReward != 0) {
            baseReward = m_genesisBlockReward;
            logger(INFO) << "Genesis block reward: " << baseReward << " de " << CryptoNote::CRYPTONOTE_NAME;
        }

        size_t blockGrantedFullRewardZone = blockGrantedFullRewardZoneByBlockVersion(blockMajorVersion);
		medianSize = std::max(medianSize, blockGrantedFullRewardZone);
		if (currentBlockSize > UINT64_C(2) * medianSize) {
			logger(TRACE) << "Block cumulative size is too big: " << currentBlockSize << ", expected less than " << 2 * medianSize;
			return false;
		}

		uint64_t penalizedBaseReward = getPenalizedAmount(baseReward, medianSize, currentBlockSize);
		uint64_t penalizedFee = blockMajorVersion >= BLOCK_MAJOR_VERSION_2 ? getPenalizedAmount(fee, medianSize, currentBlockSize) : fee;
		if (cryptonoteCoinVersion() == 1) {
			penalizedFee = getPenalizedAmount(fee, medianSize, currentBlockSize);
		}

		emissionChange = penalizedBaseReward - (fee - penalizedFee);
		reward = penalizedBaseReward + penalizedFee;

		return true;
	}

	size_t Currency::maxBlockCumulativeSize(uint64_t height) const {
		assert(height <= std::numeric_limits<uint64_t>::max() / m_maxBlockSizeGrowthSpeedNumerator);
		size_t maxSize = static_cast<size_t>(m_maxBlockSizeInitial +
			(height * m_maxBlockSizeGrowthSpeedNumerator) / m_maxBlockSizeGrowthSpeedDenominator);
		assert(maxSize >= m_maxBlockSizeInitial);
		return maxSize;
	}

	bool Currency::constructMinerTx(uint8_t blockMajorVersion, uint32_t height, size_t medianSize, uint64_t alreadyGeneratedCoins, size_t currentBlockSize,
		uint64_t fee, const AccountPublicAddress& minerAddress, Transaction& tx, const BinaryArray& extraNonce/* = BinaryArray()*/, size_t maxOuts/* = 1*/) const {

		tx.inputs.clear();
		tx.outputs.clear();
		tx.extra.clear();

		KeyPair txkey = generateKeyPair();
		addTransactionPublicKeyToExtra(tx.extra, txkey.publicKey);
		if (!extraNonce.empty()) {
			if (!addExtraNonceToTransactionExtra(tx.extra, extraNonce)) {
				return false;
			}
		}

		BaseInput in;
		in.blockIndex = height;

		uint64_t blockReward;
		int64_t emissionChange;
		if (!getBlockReward(height, blockMajorVersion, medianSize, currentBlockSize, alreadyGeneratedCoins, fee, blockReward, emissionChange)) {
			logger(INFO) << "Block is too big";
			return false;
		}

		std::vector<uint64_t> outAmounts;
		decompose_amount_into_digits(blockReward, UINT64_C(0),
			[&outAmounts](uint64_t a_chunk) { outAmounts.push_back(a_chunk); },
			[&outAmounts](uint64_t a_dust) { outAmounts.push_back(a_dust); });

		if (!(1 <= maxOuts)) { logger(ERROR, BRIGHT_RED) << "max_out must be non-zero"; return false; }
		while (maxOuts < outAmounts.size()) {
			outAmounts[outAmounts.size() - 2] += outAmounts.back();
			outAmounts.resize(outAmounts.size() - 1);
		}

		uint64_t summaryAmounts = 0;
		for (size_t no = 0; no < outAmounts.size(); no++) {
			Crypto::KeyDerivation derivation = boost::value_initialized<Crypto::KeyDerivation>();
			Crypto::PublicKey outEphemeralPubKey = boost::value_initialized<Crypto::PublicKey>();

			bool r = Crypto::generate_key_derivation(minerAddress.viewPublicKey, txkey.secretKey, derivation);

			if (!(r)) {
				logger(ERROR, BRIGHT_RED)
					<< "while creating outs: failed to generate_key_derivation("
					<< minerAddress.viewPublicKey << ", " << txkey.secretKey << ")";
				return false;
			}

			r = Crypto::derive_public_key(derivation, no, minerAddress.spendPublicKey, outEphemeralPubKey);

			if (!(r)) {
				logger(ERROR, BRIGHT_RED)
					<< "while creating outs: failed to derive_public_key("
					<< derivation << ", " << no << ", "
					<< minerAddress.spendPublicKey << ")";
				return false;
			}

			KeyOutput tk;
			tk.key = outEphemeralPubKey;

			TransactionOutput out;
			summaryAmounts += out.amount = outAmounts[no];
			out.target = tk;
			tx.outputs.push_back(out);
		}

		if (!(summaryAmounts == blockReward)) {
			logger(ERROR, BRIGHT_RED) << "Failed to construct miner tx, summaryAmounts = " << summaryAmounts << " not equal blockReward = " << blockReward;
			return false;
		}

		tx.version = CURRENT_TRANSACTION_VERSION;
		//lock
       	tx.unlockTime = height + ((height < CryptoNote::parameters::UPGRADE_HEIGHT_V4_FIX_1 && height > CryptoNote::parameters::UPGRADE_HEIGHT_V4) ? m_minedMoneyUnlockWindow_v1 : m_minedMoneyUnlockWindow);
		tx.inputs.push_back(in);
		return true;
	}

	bool Currency::isFusionTransaction(const std::vector<uint64_t>& inputsAmounts, const std::vector<uint64_t>& outputsAmounts, size_t size, uint32_t height) const {
		if (height <= CryptoNote::parameters::UPGRADE_HEIGHT_V3 ? size > CryptoNote::parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_CURRENT * 30 / 100 : size > fusionTxMaxSize()) {
			logger(ERROR) << "Fusion transaction verification failed: size exceeded max allowed size.";
			return false;
		}

		if (inputsAmounts.size() < fusionTxMinInputCount()) {
			logger(ERROR) << "Fusion transaction verification failed: inputs count is less than minimum.";
			return false;
		}

		if (inputsAmounts.size() < outputsAmounts.size() * fusionTxMinInOutCountRatio()) {
			logger(ERROR) << "Fusion transaction verification failed: inputs to outputs count ratio is less than minimum.";
			return false;
		}

		uint64_t inputAmount = 0;
		for (auto amount : inputsAmounts) {
			if (height < CryptoNote::parameters::UPGRADE_HEIGHT_V4)
				if (amount < defaultDustThreshold()) {
					logger(ERROR) << "Fusion transaction verification failed: amount " << amount << " is less than dust threshold.";
					return false;
				}
			inputAmount += amount;
		}

		std::vector<uint64_t> expectedOutputsAmounts;
		expectedOutputsAmounts.reserve(outputsAmounts.size());
		decomposeAmount(inputAmount, height < CryptoNote::parameters::UPGRADE_HEIGHT_V4 ? defaultDustThreshold() : UINT64_C(0), expectedOutputsAmounts);
		std::sort(expectedOutputsAmounts.begin(), expectedOutputsAmounts.end());

		bool decompose = expectedOutputsAmounts == outputsAmounts;
		if (!decompose) {
			logger(ERROR) << "Fusion transaction verification failed: decomposed output amounts do not match expected.";
			return false;
		}

		return true;
	}

	bool Currency::isFusionTransaction(const Transaction& transaction, size_t size, uint32_t height) const {
		assert(getObjectBinarySize(transaction) == size);

		std::vector<uint64_t> outputsAmounts;
		outputsAmounts.reserve(transaction.outputs.size());
		for (const TransactionOutput& output : transaction.outputs) {
			outputsAmounts.push_back(output.amount);
		}

		return isFusionTransaction(getInputsAmounts(transaction), outputsAmounts, size, height);
	}

	bool Currency::isFusionTransaction(const Transaction& transaction, uint32_t height) const {
		return isFusionTransaction(transaction, getObjectBinarySize(transaction), height);
	}

	bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint32_t height) const {
		uint8_t ignore;
		return isAmountApplicableInFusionTransactionInput(amount, threshold, ignore, height);
	}

	bool Currency::isAmountApplicableInFusionTransactionInput(uint64_t amount, uint64_t threshold, uint8_t& amountPowerOfTen, uint32_t height) const {
		if (amount >= threshold) {
			return false;
		}

		if (height < CryptoNote::parameters::UPGRADE_HEIGHT_V4 && amount < defaultDustThreshold()) {
			return false;
		}

		auto it = std::lower_bound(PRETTY_AMOUNTS.begin(), PRETTY_AMOUNTS.end(), amount);
		if (it == PRETTY_AMOUNTS.end() || amount != *it) {
			return false;
		}

		amountPowerOfTen = static_cast<uint8_t>(std::distance(PRETTY_AMOUNTS.begin(), it) / 9);
		return true;
	}

	std::string Currency::accountAddressAsString(const AccountBase& account) const {
		return getAccountAddressAsStr(m_publicAddressBase58Prefix, account.getAccountKeys().address);
	}

	std::string Currency::accountAddressAsString(const AccountPublicAddress& accountPublicAddress) const {
		return getAccountAddressAsStr(m_publicAddressBase58Prefix, accountPublicAddress);
	}

	bool Currency::parseAccountAddressString(const std::string& str, AccountPublicAddress& addr) const {
		uint64_t prefix;
		if (!CryptoNote::parseAccountAddressString(prefix, addr, str)) {
			return false;
		}

		if (prefix != m_publicAddressBase58Prefix) {
			logger(DEBUGGING) << "Wrong address prefix: " << prefix << ", expected " << m_publicAddressBase58Prefix;
			return false;
		}

		return true;
	}

	std::string Currency::formatAmount(uint64_t amount) const {
		std::string s = std::to_string(amount);
		if (s.size() < m_numberOfDecimalPlaces + 1) {
			s.insert(0, m_numberOfDecimalPlaces + 1 - s.size(), '0');
		}
		s.insert(s.size() - m_numberOfDecimalPlaces, ".");
		return s;
	}

	std::string Currency::formatAmount(int64_t amount) const {
		std::string s = formatAmount(static_cast<uint64_t>(std::abs(amount)));

		if (amount < 0) {
			s.insert(0, "-");
		}

		return s;
	}

	bool Currency::parseAmount(const std::string& str, uint64_t& amount) const {
		std::string strAmount = str;
		boost::algorithm::trim(strAmount);

		size_t pointIndex = strAmount.find_first_of('.');
		size_t fractionSize;
		if (std::string::npos != pointIndex) {
			fractionSize = strAmount.size() - pointIndex - 1;
			while (m_numberOfDecimalPlaces < fractionSize && '0' == strAmount.back()) {
				strAmount.erase(strAmount.size() - 1, 1);
				--fractionSize;
			}
			if (m_numberOfDecimalPlaces < fractionSize) {
				return false;
			}
			strAmount.erase(pointIndex, 1);
		}
		else {
			fractionSize = 0;
		}

		if (strAmount.empty()) {
			return false;
		}

		if (!std::all_of(strAmount.begin(), strAmount.end(), ::isdigit)) {
			return false;
		}

		if (fractionSize < m_numberOfDecimalPlaces) {
			strAmount.append(m_numberOfDecimalPlaces - fractionSize, '0');
		}

		return Common::fromString(strAmount, amount);
	}

	// Copyright (c) 2017-2018 Zawy 
	// http://zawy1.blogspot.com/2017/12/using-difficulty-to-get-constant-value.html
	// Moore's law application by Sergey Kozlov
	uint64_t Currency::getMinimalFee(uint64_t dailyDifficulty, uint64_t reward, uint64_t avgHistoricalDifficulty, uint64_t medianHistoricalReward, uint32_t height) const {
		const uint64_t blocksInTwoYears = CryptoNote::parameters::EXPECTED_NUMBER_OF_BLOCKS_PER_DAY * 365 * 2;
		const double gauge = double(0.25);
		uint64_t minimumFee(0);
		double dailyDifficultyMoore = dailyDifficulty / pow(2, static_cast<double>(height) / static_cast<double>(blocksInTwoYears));
		double minFee = gauge * CryptoNote::parameters::COIN * static_cast<double>(avgHistoricalDifficulty) 
			/ dailyDifficultyMoore * static_cast<double>(reward)
			/ static_cast<double>(medianHistoricalReward);
		if (minFee == 0 || !std::isfinite(minFee))
			return CryptoNote::parameters::MAXIMUM_FEE; // zero test 
		minimumFee = static_cast<uint64_t>(minFee);

		return std::min<uint64_t>(CryptoNote::parameters::MAXIMUM_FEE, minimumFee);
	}

	uint64_t Currency::roundUpMinFee(uint64_t minimalFee, int digits) const {
		uint64_t ret(0);
		std::string minFeeString = formatAmount(minimalFee);
		double minFee = boost::lexical_cast<double>(minFeeString);
		double scale = pow(10., floor(log10(fabs(minFee))) + (1 - digits));
		double roundedFee = ceil(minFee / scale) * scale;
		std::stringstream ss;
		ss << std::fixed << std::setprecision(12) << roundedFee;
		std::string roundedFeeString = ss.str();
		parseAmount(roundedFeeString, ret);
		return ret;
	}

	difficulty_type Currency::nextDifficulty(uint32_t height, uint8_t blockMajorVersion, std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {
		
		//std::cout << "DEBUG: BLOCK_MAJOR_VERSION = " << blockMajorVersion << std::endl;
		
		if (blockMajorVersion >= BLOCK_MAJOR_VERSION_4) {
			return nextDifficultyV4(height, timestamps, cumulativeDifficulties); // <== default Dynex difficulty for test-net
		}
		else if (blockMajorVersion == BLOCK_MAJOR_VERSION_3) {
			return nextDifficultyDefault(height, timestamps, cumulativeDifficulties);
		}
		else if (blockMajorVersion == BLOCK_MAJOR_VERSION_2) {
			return nextDifficultyDefault(height, timestamps, cumulativeDifficulties);
		}
		else {
			return nextDifficultyDefault(height, timestamps, cumulativeDifficulties); // <== default Dynex difficulty (main-net has no BLOCK_MAJOR_VERSION)
		}
	}

// Default Difficulty Calculation 
// CROAT: V1, V2, V3 Blocks

	difficulty_type Currency::nextDifficultyDefault(uint32_t blockIndex, std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {
		
		assert(m_difficultyWindow >= 2);

		if (timestamps.size() > m_difficultyWindow) {
			timestamps.resize(m_difficultyWindow);
			cumulativeDifficulties.resize(m_difficultyWindow);
		}

		size_t length = timestamps.size();
		assert(length == cumulativeDifficulties.size());
		assert(length <= m_difficultyWindow);
		if (length <= 1) {
			return 1;
		}

		sort(timestamps.begin(), timestamps.end());

		size_t cutBegin, cutEnd;
		assert(2 * m_difficultyCut <= m_difficultyWindow - 2);
		if (length <= m_difficultyWindow - 2 * m_difficultyCut) {
			cutBegin = 0;
			cutEnd = length;
		}
		else {
			cutBegin = (length - (m_difficultyWindow - 2 * m_difficultyCut) + 1) / 2;
			cutEnd = cutBegin + (m_difficultyWindow - 2 * m_difficultyCut);
		}
		assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
		uint64_t timeSpan = timestamps[cutEnd - 1] - timestamps[cutBegin];
		if (timeSpan == 0) {
			timeSpan = 1;
		}

		difficulty_type totalWork = cumulativeDifficulties[cutEnd - 1] - cumulativeDifficulties[cutBegin];
		assert(totalWork > 0);

		uint64_t low, high;
		low = mul128(totalWork, m_difficultyTarget, &high);
		if (high != 0 || low + timeSpan - 1 < low) {
			return 0;
		}

		uint64_t nextDiffDefault = (low + timeSpan - 1) / timeSpan;
                
		// Testnet hardcode
		if (isTestnet()) { nextDiffDefault = nextDiffDefault/2; }

		// DynexSolve - adjust difficulty for new mining algorithm
		// mined blocks will be used for aidrop
		if (!isTestnet() && (blockIndex>=58494 && blockIndex<=58494+725)) {
				nextDiffDefault = 14;
				logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiffDefault << "' in mainnet.";
		}
		// Dynex 213 - adjust difficulty
		if (!isTestnet() && (blockIndex>=67589 && blockIndex<=67589+725)) {
				nextDiffDefault = 60000000;
				logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiffDefault << "' in mainnet.";
		}
		// Dynex 220
		if (!isTestnet() && (blockIndex>=70336 && blockIndex<=70345)) {
				nextDiffDefault = 60000000;
				logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiffDefault << "' in mainnet.";
		}
		
		// Dynex 221
		if (!isTestnet() && (blockIndex>=70346 && blockIndex<=70346+50)) {
				nextDiffDefault = 5000000; 
				logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiffDefault << "' in mainnet.";
		}
		
		if (!isTestnet() && (blockIndex>=70404 && blockIndex<=70404+725)) {
				nextDiffDefault = 5000000; 
				logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiffDefault << "' in mainnet.";
		}

        return nextDiffDefault;
	}

////////////////////////////////////////////
// V4 Blocks
// DEFAULT DIFFICULTY CALCULATION FOR DYNEX

	difficulty_type Currency::nextDifficultyV4(uint32_t blockIndex, std::vector<uint64_t> timestamps,
		std::vector<difficulty_type> cumulativeDifficulties) const {
		assert(m_difficultyWindow >= 2);

		// Hardcode difficulty for N blocks after fork height if no isTestnet
        const size_t N = CryptoNote::parameters::DIFFICULTY_WINDOW_V4;
        if ((blockIndex >= upgradeHeight(CryptoNote::BLOCK_MAJOR_VERSION_4) && blockIndex <= (upgradeHeight(CryptoNote::BLOCK_MAJOR_VERSION_4) + N))) {
            
            uint32_t offset = N - (blockIndex - upgradeHeight(CryptoNote::BLOCK_MAJOR_VERSION_4));
            
            if (!isTestnet())
            {
            	uint64_t nextDiff = 17141714; // Fixed difficulty value
                logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiff << "' in mainnet! " << offset << " blocks left!";
                return nextDiff;
            }
            else if (isTestnet()) //Hardcode difficulty if isTestnet
            {
            		uint64_t nextDiff = 14;
                	logger(DEBUGGING, BRIGHT_YELLOW) << "Fixing Diff to '" << nextDiff << "' in testnet! " << offset << " blocks left!";
                	return nextDiff;
            }
        }
        
		if (timestamps.size() > m_difficultyWindow) {
			timestamps.resize(m_difficultyWindow);
			cumulativeDifficulties.resize(m_difficultyWindow);
		}

		size_t length = timestamps.size();
		assert(length == cumulativeDifficulties.size());
		assert(length <= m_difficultyWindow);
		if (length <= 1) {
			return 1;
		}

		sort(timestamps.begin(), timestamps.end());

		size_t cutBegin, cutEnd;
		assert(2 * m_difficultyCut <= m_difficultyWindow - 2);
		if (length <= m_difficultyWindow - 2 * m_difficultyCut) {
			cutBegin = 0;
			cutEnd = length;
		}
		else {
			cutBegin = (length - (m_difficultyWindow - 2 * m_difficultyCut) + 1) / 2;
			cutEnd = cutBegin + (m_difficultyWindow - 2 * m_difficultyCut);
		}
		assert(/*cut_begin >= 0 &&*/ cutBegin + 2 <= cutEnd && cutEnd <= length);
		uint64_t timeSpan = timestamps[cutEnd - 1] - timestamps[cutBegin];
		if (timeSpan == 0) {
			timeSpan = 1;
		}

		difficulty_type totalWork = cumulativeDifficulties[cutEnd - 1] - cumulativeDifficulties[cutBegin];
		assert(totalWork > 0);

		uint64_t low, high;
		low = mul128(totalWork, m_difficultyTarget, &high);
		if (high != 0 || low + timeSpan - 1 < low) {
			return 0;
		}

		uint64_t nextDiff = (low + timeSpan - 1) / timeSpan;
        
		// minimum limit
		if (!isTestnet() && nextDiff < 100000) {
				nextDiff = 100000;
		}
		
        // Testnet hardcode
        /*
        if (isTestnet()) { nextDiff = nextDiff/2; }	
		*/
	
		// DYNEXSOLVE: TEST-NET ADJUST DIFFICULTY AT BLOCK xxx FOR DYNEXSOLVE
		if (isTestnet() && (blockIndex>=17292 && blockIndex<=17493)) { //testnet version 2.0.6 set back difficulty window = 200
        	uint64_t nextDiff = 14;
        	logger(DEBUGGING, BRIGHT_YELLOW) << "DynexSolve 2.0.6: BLOCK 17292-17493 Fixing Diff to '" << nextDiff << "' in testnet! ";
        	return nextDiff;
        }
        if (isTestnet() && blockIndex < 17292) { 
        	nextDiff = nextDiff/2; 
        	if (nextDiff < 14) nextDiff = 14;
        }
        
    return nextDiff;          
}
	


// Proof of Work
//

/* checkProofOfWorkV1 */

bool Currency::checkProofOfWorkV1(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic,
	Crypto::Hash& proofOfWork) const {
	if (BLOCK_MAJOR_VERSION_2 == block.majorVersion || BLOCK_MAJOR_VERSION_3 == block.majorVersion) {
		return false;
	}
//std::cout << "*** DEBUG *** Currency.cpp -> checkProofOfWorkV1" << std::endl; // only for genesis block
	if (!get_block_longhash(context, block, proofOfWork)) {
		return false;
	}
	return check_hash(proofOfWork, currentDiffic);
}

/* checkProofOfWorkV2 */

bool Currency::checkProofOfWorkV2(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic,
		Crypto::Hash& proofOfWork) const {

    //std::cout << "*** DEBUG *** Currency.cpp -> checkProofOfWorkV2" << std::endl; // <=== MAIN VERIFICATION OF MINED BLOCK

		if (block.majorVersion < BLOCK_MAJOR_VERSION_2) {
			return false;
		}

		if (!get_block_longhash(context, block, proofOfWork)) {
			return false;
		}

		if (!check_hash(proofOfWork, currentDiffic)) {
			return false;
		}

		TransactionExtraMergeMiningTag mmTag;
		if (!getMergeMiningTagFromExtra(block.parentBlock.baseTransaction.extra, mmTag)) {
			logger(ERROR) << "merge mining tag wasn't found in extra of the parent block miner transaction";
			return false;
		}

		if (8 * sizeof(m_genesisBlockHash) < block.parentBlock.blockchainBranch.size()) {
			return false;
		}

		Crypto::Hash auxBlockHeaderHash;
		if (!get_aux_block_header_hash(block, auxBlockHeaderHash)) {
			return false;
		}

		Crypto::Hash auxBlocksMerkleRoot;
		Crypto::tree_hash_from_branch(block.parentBlock.blockchainBranch.data(), block.parentBlock.blockchainBranch.size(),
			auxBlockHeaderHash, &m_genesisBlockHash, auxBlocksMerkleRoot);

		if (auxBlocksMerkleRoot != mmTag.merkleRoot) {
			logger(ERROR, BRIGHT_YELLOW) << "Aux block hash wasn't found in merkle tree";
			return false;
		}

		return true;
	}

	bool Currency::checkProofOfWork(Crypto::cn_context& context, const Block& block, difficulty_type currentDiffic, Crypto::Hash& proofOfWork) const {
		switch (block.majorVersion) {
		case BLOCK_MAJOR_VERSION_1:
		case BLOCK_MAJOR_VERSION_4:
			return checkProofOfWorkV1(context, block, currentDiffic, proofOfWork);

		case BLOCK_MAJOR_VERSION_2:
		case BLOCK_MAJOR_VERSION_3:
			return checkProofOfWorkV2(context, block, currentDiffic, proofOfWork);
		}

		logger(ERROR, BRIGHT_RED) << "Unknown block major version: " << block.majorVersion << "." << block.minorVersion;
		return false;
	}

	size_t Currency::getApproximateMaximumInputCount(size_t transactionSize, size_t outputCount, size_t mixinCount) const {
		const size_t KEY_IMAGE_SIZE = sizeof(Crypto::KeyImage);
		const size_t OUTPUT_KEY_SIZE = sizeof(decltype(KeyOutput::key));
		const size_t AMOUNT_SIZE = sizeof(uint64_t) + 2; //varint
		const size_t GLOBAL_INDEXES_VECTOR_SIZE_SIZE = sizeof(uint8_t);//varint
		const size_t GLOBAL_INDEXES_INITIAL_VALUE_SIZE = sizeof(uint32_t);//varint
		const size_t GLOBAL_INDEXES_DIFFERENCE_SIZE = sizeof(uint32_t);//varint
		const size_t SIGNATURE_SIZE = sizeof(Crypto::Signature);
		const size_t EXTRA_TAG_SIZE = sizeof(uint8_t);
		const size_t INPUT_TAG_SIZE = sizeof(uint8_t);
		const size_t OUTPUT_TAG_SIZE = sizeof(uint8_t);
		const size_t PUBLIC_KEY_SIZE = sizeof(Crypto::PublicKey);
		const size_t TRANSACTION_VERSION_SIZE = sizeof(uint8_t);
		const size_t TRANSACTION_UNLOCK_TIME_SIZE = sizeof(uint64_t);

		const size_t outputsSize = outputCount * (OUTPUT_TAG_SIZE + OUTPUT_KEY_SIZE + AMOUNT_SIZE);
		const size_t headerSize = TRANSACTION_VERSION_SIZE + TRANSACTION_UNLOCK_TIME_SIZE + EXTRA_TAG_SIZE + PUBLIC_KEY_SIZE;
		const size_t inputSize = INPUT_TAG_SIZE + AMOUNT_SIZE + KEY_IMAGE_SIZE + SIGNATURE_SIZE + GLOBAL_INDEXES_VECTOR_SIZE_SIZE + GLOBAL_INDEXES_INITIAL_VALUE_SIZE +
			mixinCount * (GLOBAL_INDEXES_DIFFERENCE_SIZE + SIGNATURE_SIZE);

		return (transactionSize - headerSize - outputsSize) / inputSize;
	}

	CurrencyBuilder::CurrencyBuilder(Logging::ILogger& log) : m_currency(log) {
		maxBlockNumber(parameters::CRYPTONOTE_MAX_BLOCK_NUMBER);
		maxBlockBlobSize(parameters::CRYPTONOTE_MAX_BLOCK_BLOB_SIZE);
		maxTxSize(parameters::CRYPTONOTE_MAX_TX_SIZE);
		publicAddressBase58Prefix(parameters::CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
		minedMoneyUnlockWindow(parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);
		minedMoneyUnlockWindow_v1(parameters::CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW_V1);
		transactionSpendableAge(parameters::CRYPTONOTE_TX_SPENDABLE_AGE);
		expectedNumberOfBlocksPerDay(parameters::EXPECTED_NUMBER_OF_BLOCKS_PER_DAY);

		timestampCheckWindow(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);
		timestampCheckWindow_v1(parameters::BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1);
		blockFutureTimeLimit(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT);
		blockFutureTimeLimit_v1(parameters::CRYPTONOTE_BLOCK_FUTURE_TIME_LIMIT_V1);

		moneySupply(parameters::MONEY_SUPPLY);
		emissionSpeedFactor(parameters::EMISSION_SPEED_FACTOR);
		genesisBlockReward(parameters::GENESIS_BLOCK_REWARD);        
		cryptonoteCoinVersion(parameters::CRYPTONOTE_COIN_VERSION);

		rewardBlocksWindow(parameters::CRYPTONOTE_REWARD_BLOCKS_WINDOW);
		blockGrantedFullRewardZone(parameters::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE);
		minerTxBlobReservedSize(parameters::CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
		maxTransactionSizeLimit(parameters::MAX_TRANSACTION_SIZE_LIMIT);

		minMixin(parameters::MIN_TX_MIXIN_SIZE);
		maxMixin(parameters::MAX_TX_MIXIN_SIZE);

		numberOfDecimalPlaces(parameters::CRYPTONOTE_DISPLAY_DECIMAL_POINT);

		minimumFee(parameters::MINIMUM_FEE);
		defaultDustThreshold(parameters::DEFAULT_DUST_THRESHOLD);

		difficultyTarget(parameters::DIFFICULTY_TARGET);
		difficultyWindow(parameters::DIFFICULTY_WINDOW);
		difficultyLag(parameters::DIFFICULTY_LAG);
		difficultyCut(parameters::DIFFICULTY_CUT);

		maxBlockSizeInitial(parameters::MAX_BLOCK_SIZE_INITIAL);
		maxBlockSizeGrowthSpeedNumerator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_NUMERATOR);
		maxBlockSizeGrowthSpeedDenominator(parameters::MAX_BLOCK_SIZE_GROWTH_SPEED_DENOMINATOR);

		lockedTxAllowedDeltaSeconds(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS);
		lockedTxAllowedDeltaBlocks(parameters::CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS);

		mempoolTxLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_LIVETIME);
		mempoolTxFromAltBlockLiveTime(parameters::CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME);
		numberOfPeriodsToForgetTxDeletedFromPool(parameters::CRYPTONOTE_NUMBER_OF_PERIODS_TO_FORGET_TX_DELETED_FROM_POOL);

		fusionTxMaxSize(parameters::FUSION_TX_MAX_SIZE);
		fusionTxMinInputCount(parameters::FUSION_TX_MIN_INPUT_COUNT);
		fusionTxMinInOutCountRatio(parameters::FUSION_TX_MIN_IN_OUT_COUNT_RATIO);

		upgradeHeightV2(parameters::UPGRADE_HEIGHT_V2);
		upgradeHeightV3(parameters::UPGRADE_HEIGHT_V3);
		upgradeHeightV4(parameters::UPGRADE_HEIGHT_V4);

		upgradeVotingThreshold(parameters::UPGRADE_VOTING_THRESHOLD);
		upgradeVotingWindow(parameters::UPGRADE_VOTING_WINDOW);
		upgradeWindow(parameters::UPGRADE_WINDOW);

		blocksFileName(parameters::CRYPTONOTE_BLOCKS_FILENAME);
		blocksCacheFileName(parameters::CRYPTONOTE_BLOCKSCACHE_FILENAME);
		blockIndexesFileName(parameters::CRYPTONOTE_BLOCKINDEXES_FILENAME);
		txPoolFileName(parameters::CRYPTONOTE_POOLDATA_FILENAME);
		blockchainIndicesFileName(parameters::CRYPTONOTE_BLOCKCHAIN_INDICES_FILENAME);

		testnet(false);
	}

	Transaction CurrencyBuilder::generateGenesisTransaction() {
		CryptoNote::Transaction tx;
		CryptoNote::AccountPublicAddress ac = boost::value_initialized<CryptoNote::AccountPublicAddress>();
		m_currency.constructMinerTx(1, 0, 0, 0, 0, 0, ac, tx); // zero fee in genesis
		return tx;
}
 Transaction CurrencyBuilder::generateGenesisTransaction(const std::vector<AccountPublicAddress>& targets) {
    assert(!targets.empty());
 
    CryptoNote::Transaction tx;
    tx.inputs.clear();
    tx.outputs.clear();
    tx.extra.clear();
    tx.version = CURRENT_TRANSACTION_VERSION;
    tx.unlockTime = m_currency.m_minedMoneyUnlockWindow;
    KeyPair txkey = generateKeyPair();
    addTransactionPublicKeyToExtra(tx.extra, txkey.publicKey);
    BaseInput in;
    in.blockIndex = 0;
    tx.inputs.push_back(in);
    uint64_t block_reward = m_currency.m_genesisBlockReward;
    uint64_t target_amount = block_reward / targets.size();
    uint64_t first_target_amount = target_amount + block_reward % targets.size();
    for (size_t i = 0; i < targets.size(); ++i) {
      Crypto::KeyDerivation derivation = boost::value_initialized<Crypto::KeyDerivation>();
      Crypto::PublicKey outEphemeralPubKey = boost::value_initialized<Crypto::PublicKey>();
      bool r = Crypto::generate_key_derivation(targets[i].viewPublicKey, txkey.secretKey, derivation);
      assert(r == true);
//      CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << targets[i].viewPublicKey << ", " << txkey.sec << ")");
      r = Crypto::derive_public_key(derivation, i, targets[i].spendPublicKey, outEphemeralPubKey);
      assert(r == true);
 //     CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << i << ", " << targets[i].spendPublicKey << ")");
      KeyOutput tk;
      tk.key = outEphemeralPubKey;
      TransactionOutput out;
      out.amount = (i == 0) ? first_target_amount : target_amount;
      std::cout << "outs: " << std::to_string(out.amount) << std::endl;
      out.target = tk;
      tx.outputs.push_back(out);
    }
    return tx;
}
	CurrencyBuilder& CurrencyBuilder::emissionSpeedFactor(unsigned int val) {
		if (val <= 0 || val > 8 * sizeof(uint64_t)) {
			throw std::invalid_argument("val at emissionSpeedFactor()");
		}

		m_currency.m_emissionSpeedFactor = val;
		return *this;
	}

	CurrencyBuilder& CurrencyBuilder::numberOfDecimalPlaces(size_t val) {
		m_currency.m_numberOfDecimalPlaces = val;
		m_currency.m_coin = 1;
		for (size_t i = 0; i < m_currency.m_numberOfDecimalPlaces; ++i) {
			m_currency.m_coin *= 10;
		}

		return *this;
	}

	CurrencyBuilder& CurrencyBuilder::difficultyWindow(size_t val) {
		if (val < 2) {
			throw std::invalid_argument("val at difficultyWindow()");
		}
		m_currency.m_difficultyWindow = val;
		return *this;
	}

	CurrencyBuilder& CurrencyBuilder::upgradeVotingThreshold(unsigned int val) {
		if (val <= 0 || val > 100) {
			throw std::invalid_argument("val at upgradeVotingThreshold()");
		}

		m_currency.m_upgradeVotingThreshold = val;
		return *this;
	}

	CurrencyBuilder& CurrencyBuilder::upgradeWindow(size_t val) {
		if (val <= 0) {
			throw std::invalid_argument("val at upgradeWindow()");
		}

		m_currency.m_upgradeWindow = static_cast<uint32_t>(val);
		return *this;
	}

}
