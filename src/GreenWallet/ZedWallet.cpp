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


////////////////////////////////
#include <GreenWallet/ZedWallet.h>
////////////////////////////////

#include <Common/SignalHandler.h>

#include <CryptoNoteCore/Currency.h>

#include <Logging/FileLogger.h>
#include <Logging/LoggerManager.h>

#include <clocale>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GreenWallet/ColouredMsg.h>
#include <GreenWallet/Menu.h>
#include <GreenWallet/ParseArguments.h>
#include <GreenWallet/Tools.h>
#include <GreenWallet/WalletConfig.h>

int main(int argc, char **argv)
{
	/* Fix wallet not responding when enter cyrillic (non-latin) characters 
	   by setting operating system default locale.
	   Set cyrillic locale on Windows explicitly - this is Karbovanets after all. */
	setlocale(LC_CTYPE, "");
#ifdef WIN32
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
#endif
    /* On ctrl+c the program seems to throw "zedwallet.exe has stopped
       working" when calling exit(0)... I'm not sure why, this is a bit of
       a hack, it disables that - possibly some deconstructers calling
       terminate() */
    #ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    #endif

    Config config = parseArguments(argc, argv);

    /* User requested --help or --version, or invalid arguments */
    if (config.exit)
    {
        return 0;
    }

    Logging::LoggerManager logManager;

    /* Currency contains our coin parameters, such as decimal places, supply */
    const CryptoNote::Currency currency 
        = CryptoNote::CurrencyBuilder(logManager).currency();

	System::Dispatcher dispatcher;

    /* Our connection to daemon */
	CryptoNote::INode* node = new CryptoNote::NodeRpcProxy(config.host, config.port);
	std::unique_ptr<CryptoNote::INode> nodeGuard(node);

    std::promise<std::error_code> errorPromise;

    /* Once the function is complete, set the error value from the promise */
    auto callback = [&errorPromise](std::error_code e)
    {
        errorPromise.set_value(e);
    };

    /* Get the future of the result */
    auto initNode = errorPromise.get_future();

    node->init(callback);

    /* Connection took to long to remote node, let program continue regardless
       as they could perform functions like export_keys without being
       connected */
    if (initNode.wait_for(std::chrono::seconds(20)) != std::future_status::ready)
    {
        if (config.host != "127.0.0.1")
        {
            std::cout << WarningMsg("Unable to connect to remote node, "
                                    "connection timed out.")
                      << std::endl
                      << WarningMsg("Confirm the remote node is functioning, "
                                    "or try a different remote node.")
                      << std::endl << std::endl;
        }
        else
        {
            std::cout << WarningMsg("Unable to connect to node, "
                                    "connection timed out.")
                      << std::endl << std::endl;
        }
    }

	/*
	  This will check to see if the node responded to /feeaddress and actually
	  returned something that it expects us to use for convenience charges
	  for using that node to send transactions.
	*/
	if (!node->feeAddress().empty()) {
		std::stringstream feemsg;

		feemsg << std::endl << "You have connected to a node that charges " <<
			"a fee to send transactions." << std::endl << std::endl
			<< "The fee for sending transactions is 0.25% of transaction amount, " <<
			"but no more than " << formatAmount(1000000000000) << "DNX." 
			<< std::endl << std::endl <<
			"If you don't want to pay the node fee, please " <<
			"relaunch " << WalletConfig::walletName <<
			" and run your own node." <<
			std::endl;

		std::cout << WarningMsg(feemsg.str()) << std::endl;
	}

    /* Create the wallet instance */
	CryptoNote::WalletGreen* wallet = new CryptoNote::WalletGreen(dispatcher, currency, *node, logManager);
	std::unique_ptr<CryptoNote::WalletGreen> walletGuard(wallet);

    /* Run the interactive wallet interface */
    run(*wallet, *node, config);
}

void run(CryptoNote::WalletGreen &wallet, CryptoNote::INode &node,
	Config &config)
{
	std::cout << InformationMsg(getVersion()) << std::endl;

	std::shared_ptr<WalletInfo> walletInfo;

	bool quit;

	std::tie(quit, walletInfo) = selectionScreen(config, wallet, node);

	bool alreadyShuttingDown = false;

	if (!quit)
	{
		/* Call shutdown on ctrl+c */
		Tools::SignalHandler::install([&]
		{
			/* If we're already shutting down let control flow continue
			   as normal */
			if (shutdown(walletInfo, node, alreadyShuttingDown))
			{
				exit(0);
			}
		});

		mainLoop(walletInfo, node);
	}

	shutdown(walletInfo, node, alreadyShuttingDown);
}
