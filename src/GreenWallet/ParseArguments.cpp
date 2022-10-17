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


/////////////////////////////////////
#include <GreenWallet/ParseArguments.h>
/////////////////////////////////////

#include "CryptoNoteConfig.h"

#include <iomanip>
#include <initializer_list>
#include <iostream>

#include "version.h"

#include <Common/UrlTools.h>
#include <GreenWallet/WalletConfig.h>

/* Thanks to https://stackoverflow.com/users/85381/iain for this small command
   line parsing snippet! https://stackoverflow.com/a/868894/8737306 */
char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    auto it = std::find(begin, end, option);

    if (it != end && ++it != end)
    {
        return *it;
    }

    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

Config parseArguments(int argc, char **argv)
{
    Config config;

    if (cmdOptionExists(argv, argv+argc, "-h")
     || cmdOptionExists(argv, argv+argc, "--help"))
    {
        helpMessage();
        config.exit = true;
        return config;
    }

    if (cmdOptionExists(argv, argv+argc, "-v")
     || cmdOptionExists(argv, argv+argc, "--version"))
    {
        std::cout << getVersion() << std::endl;
        config.exit = true;
        return config;
    }

    if (cmdOptionExists(argv, argv+argc, "--wallet-file"))
    {
        char *wallet = getCmdOption(argv, argv+argc, "--wallet-file");

        if (!wallet)
        {
            std::cout << "--wallet-file was specified, but no wallet file "
                      << "was given!" << std::endl;

            helpMessage();
            config.exit = true;
            return config;
        }

        config.walletFile = std::string(wallet);
        config.walletGiven = true;
    }

    if (cmdOptionExists(argv, argv+argc, "--password"))
    {
        char *password = getCmdOption(argv, argv+argc, "--password");

        if (!password)
        {
            std::cout << "--password was specified, but no password was "
                      << "given!" << std::endl;

            helpMessage();
            config.exit = true;
            return config;
        }

        config.walletPass = std::string(password);
        config.passGiven = true;
    }

    if (cmdOptionExists(argv, argv+argc, "--remote-daemon"))
    {
        char *url = getCmdOption(argv, argv + argc, "--remote-daemon");

        /* No url following --remote-daemon */
        if (!url)
        {
            std::cout << "--remote-daemon was specified, but no daemon was "
                      << "given!" << std::endl;

            helpMessage();

            config.exit = true;
        }
        else
        {
            std::string urlString(url);

            if (!Common::parseUrlAddress(urlString, config.host, config.port,
                                         config.path, config.ssl)) {

                std::cout << "Failed to parse daemon address!" << std::endl;
                config.exit = true;
            }

        }
    }

    return config;
}

std::string getVersion()
{
    return WalletConfig::coinName + " v" + CN_PROJECT_VERSION + " "
         + WalletConfig::walletName;
}

std::vector<CLICommand> getCLICommands()
{
    std::vector<CLICommand> commands =
    {
        {"--help", "Display this help message and exit", "-h", true, false},

        {"--version", "Display the version information and exit", "-v", true,
         false},

        {"--remote-daemon <url>", "Connect to the remote daemon at <url>", "",
         false, true},

        {"--wallet-file <file>", "Open the wallet <file>", "", false, true},

        {"--password <pass>", "Use the password <pass> to open the wallet", "",
         false, true}
    };

    /* Pop em in alphabetical order */
    std::sort(commands.begin(), commands.end(), [](const CLICommand &lhs,
                                                   const CLICommand &rhs)
    {
        return lhs.name < rhs.name;
    });


    return commands;
}

void helpMessage()
{
    std::cout << getVersion() << std::endl;

    const auto commands = getCLICommands();

    std::cout << std::endl
              << WalletConfig::walletName;

    for (auto &command : commands)
    {
        if (command.hasArgument)
        {
            std::cout << " [" << command.name << "]";
        }
    }

    std::cout << std::endl << std::endl
              << "Commands: " << std::endl;

    for (auto &command : commands)
    {
        if (command.hasShortName)
        {
            std::cout << "  " << command.shortName << ", "
                      << std::left << std::setw(25) << command.name
                      << command.description << std::endl;
        }
        else
        {
            std::cout << "      " << std::left << std::setw(25) << command.name
                      << command.description << std::endl;
        }
    }
}
