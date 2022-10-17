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


///////////////////////////
#include <GreenWallet/Sync.h>
///////////////////////////

#include <Common/StringTools.h>

#include <iostream>

#include <GreenWallet/ColouredMsg.h>
#include <GreenWallet/CommandImplementations.h>
#include <GreenWallet/GetInput.h>
#include <GreenWallet/Tools.h>
#include <GreenWallet/Types.h>
#include <GreenWallet/WalletConfig.h>

void checkForNewTransactions(std::shared_ptr<WalletInfo> walletInfo)
{
    walletInfo->wallet.updateInternalCache();

    const size_t newTransactionCount = walletInfo->wallet.getTransactionCount();

    if (newTransactionCount != walletInfo->knownTransactionCount)
    {
        for (size_t i = walletInfo->knownTransactionCount; 
                    i < newTransactionCount; i++)
        {
            const CryptoNote::WalletTransaction t 
                = walletInfo->wallet.getTransaction(i);

            /* Don't print outgoing or fusion transfers */
            if (t.totalAmount > 0)
            {
                std::cout << std::endl
                          << InformationMsg("New transaction found!")
                          << std::endl
                          << SuccessMsg("Incoming transfer:")
                          << std::endl
                          << SuccessMsg("Hash: " + Common::podToHex(t.hash))
                          << std::endl
                          << SuccessMsg("Amount: "
                                      + formatAmount(t.totalAmount))
                          << std::endl
                          << InformationMsg(getPrompt(walletInfo))
                          << std::flush;
            }
        }

        walletInfo->knownTransactionCount = newTransactionCount;
    }
}

void syncWallet(CryptoNote::INode &node,
                std::shared_ptr<WalletInfo> walletInfo)
{
    uint32_t localHeight = node.getLastLocalBlockHeight();
    uint32_t walletHeight = walletInfo->wallet.getBlockCount();
    uint32_t remoteHeight = node.getLastKnownBlockHeight();

    size_t transactionCount = walletInfo->wallet.getTransactionCount();

    int stuckCounter = 0;

    if (localHeight != remoteHeight)
    {
        std::cout << "Your " << WalletConfig::daemonName << " isn't fully "
                  << "synced yet!" << std::endl
                  << "Until you are fully synced, you won't be able to send "
                  << "transactions,"
                  << std::endl
                  << "and your balance may be missing or "
                  << "incorrect!" << std::endl << std::endl;
    }

    /* If we open a legacy wallet then it will load the transactions but not
       have the walletHeight == transaction height. Lets just throw away the
       transactions and rescan. */
    if (walletHeight == 1 && transactionCount != 0)
    {
        std::cout << "Upgrading your wallet from an older version of the "
                  << "software..." << std::endl << "Unfortunately, we have "
                  << "to rescan the chain to find your transactions."
                  << std::endl;

        transactionCount = 0;

		// WalletGreen will handle it
		transactionCount = walletInfo->wallet.getTransactionCount();
		walletInfo->knownTransactionCount = transactionCount;
		walletHeight = walletInfo->wallet.getTransaction(transactionCount - 1).blockHeight;
        //walletInfo->wallet.clearCaches();
    }

    if (walletHeight == 1)
    {
        std::cout << "Scanning through the blockchain to find transactions "
                  << "that belong to you." << std::endl
                  << "Please wait, this will take some time."
                  << std::endl << std::endl;
    }
    else
    {
        std::cout << "Scanning through the blockchain to find any new "
                  << "transactions you received"
                  << std::endl
                  << "whilst your wallet wasn't open."
                  << std::endl
                  << "Please wait, this may take some time."
                  << std::endl << std::endl;
    }

    int counter = 1;

    while (walletHeight < localHeight)
    {
        /* This MUST be called on the main thread! */
        walletInfo->wallet.updateInternalCache();

        localHeight = node.getLastLocalBlockHeight();
        remoteHeight = node.getLastKnownBlockHeight();
        std::cout << SuccessMsg(std::to_string(walletHeight))
                  << " of " << InformationMsg(std::to_string(localHeight))
                  << "\r" << std::flush;

        const uint32_t tmpWalletHeight = walletInfo->wallet.getBlockCount();

        int waitSeconds = 1;

        /* Save periodically so if someone closes before completion they don't
           lose all their progress */
        if (counter % 600 == 0)
        {
            std::cout << std::endl
                      << InformationMsg("Saving current progress...")
                      << std::endl << std::endl;

            walletInfo->wallet.save();
        }

        if (tmpWalletHeight == walletHeight)
        {
            stuckCounter++;
            waitSeconds = 3;

            if (stuckCounter > 20)
            {
                std::cout << WarningMsg("Syncing may be stuck. Try restarting ")
                          << WarningMsg(WalletConfig::daemonName)
                          << WarningMsg(".")
                          << std::endl
                          << WarningMsg("If this persists, visit ")
                          << WarningMsg(WalletConfig::contactLink)
                          << WarningMsg(" for support.")
                          << std::endl;
            }
            else if (stuckCounter > 19)
            {
                /*
                   Calling save has the side-effect of starting
                   and stopping blockchainSynchronizer, which seems
                   to sometimes force the sync to resume properly.
                   So we'll try this before warning the user.
                */
				walletInfo->wallet.save();
                waitSeconds = 5;
            }
        }
        else
        {
            stuckCounter = 0;
            walletHeight = tmpWalletHeight;

            const size_t tmpTransactionCount = walletInfo
                                             ->wallet.getTransactionCount();

            if (tmpTransactionCount != transactionCount)
            {
                for (size_t i = transactionCount; i < tmpTransactionCount; i++)
                {
                    CryptoNote::WalletTransaction t
                        = walletInfo->wallet.getTransaction(i);

                    /* Don't print out fusion transactions */
                    if (t.totalAmount != 0)
                    {
                        std::cout << std::endl
                                  << InformationMsg("New transaction found!")
                                  << std::endl << std::endl;

                        if (t.totalAmount < 0)
                        {
                            printOutgoingTransfer(t, node);
                        }
                        else
                        {
                            printIncomingTransfer(t, node);
                        }
                    }
                }

                transactionCount = tmpTransactionCount;
            }
        }

        counter++;

        std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));
    }

    std::cout << std::endl
              << SuccessMsg("Finished scanning blockchain!") << std::endl;

    /* In case the user force closes, we don't want them to have to rescan
       the whole chain. */
	walletInfo->wallet.save();

    walletInfo->knownTransactionCount = transactionCount;
}
