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


#include <initializer_list>

//////////////////////////////////
#include <GreenWallet/AddressBook.h>
//////////////////////////////////

#include <boost/algorithm/string.hpp>

#ifndef MSVC
#include <fstream>
#endif

#include <Serialization/SerializationTools.h>

#include <GreenWallet/ColouredMsg.h>
#include <GreenWallet/Tools.h>
#include <GreenWallet/Transfer.h>
#include <GreenWallet/WalletConfig.h>

const std::string getAddressBookName(AddressBook addressBook)
{
    while (true)
    {
        std::string friendlyName;

        std::cout << InformationMsg("What friendly name do you want to ")
                  << InformationMsg("give this address book entry?: ");

        std::getline(std::cin, friendlyName);
        boost::algorithm::trim(friendlyName);

        const auto it = std::find(addressBook.begin(), addressBook.end(),
                            AddressBookEntry(friendlyName));

        if (it != addressBook.end())
        {
            std::cout << WarningMsg("An address book entry with this ")
                      << WarningMsg("name already exists!")
                      << std::endl << std::endl;

            continue;
        }

        return friendlyName;
    }
}

const Maybe<const std::string> getAddressBookAddress()
{
    std::cout << std::endl;

    while (true)
    {
        std::string transferAddr;

        std::cout << InformationMsg("What address does this user have?: ");

        std::getline(std::cin, transferAddr);
        boost::algorithm::trim(transferAddr);

        if (transferAddr == "cancel")
        {
            return Nothing<const std::string>();
        }

        if (parseAddress(transferAddr))
        {
            return Just<const std::string>(transferAddr);
        }
    }
}

const Maybe<std::string> getAddressBookPaymentID()
{
    std::stringstream msg;

    msg << std::endl
        << "Does this address book entry have a payment ID associated with it?"
        << std::endl;

    return getPaymentID(msg.str());
}

void addToAddressBook()
{
    std::cout << InformationMsg("Note: You can type cancel at any time to "
                                "cancel adding someone to your address book")
              << std::endl << std::endl;

    auto addressBook = getAddressBook();

    const std::string friendlyName = getAddressBookName(addressBook);

    if (friendlyName == "cancel")
    {
        std::cout << WarningMsg("Cancelling addition to address book.")
                  << std::endl;
        return;
    }

    auto maybeAddress = getAddressBookAddress();

    if (!maybeAddress.isJust)
    {
        std::cout << WarningMsg("Cancelling addition to address book.")
                  << std::endl;
        return;
    }

    auto maybePaymentID = getAddressBookPaymentID();

    if (!maybePaymentID.isJust)
    {
        std::cout << WarningMsg("Cancelling addition to address book.")
                  << std::endl;
    }

    addressBook.emplace_back(friendlyName, maybeAddress.x, maybePaymentID.x);

    if (saveAddressBook(addressBook))
    {
        std::cout << std::endl
                  << SuccessMsg("A new entry has been added to your address ")
                  << SuccessMsg("book!")
                  << std::endl;
    }
}

const Maybe<const AddressBookEntry> getAddressBookEntry(AddressBook addressBook)
{
    while (true)
    {
        std::string friendlyName;

        std::cout << InformationMsg("Who do you want to send to from your ")
                  << InformationMsg("address book?: ");

        std::getline(std::cin, friendlyName);
        boost::algorithm::trim(friendlyName);

        if (friendlyName == "cancel")
        {
            return Nothing<const AddressBookEntry>();
        }

        auto it = std::find(addressBook.begin(), addressBook.end(),
                            AddressBookEntry(friendlyName));

        if (it != addressBook.end())
        {
            return Just<const AddressBookEntry>(*it);
        }

        std::cout << std::endl
                  << WarningMsg("Could not find a user with the name of ")
                  << InformationMsg(friendlyName)
                  << WarningMsg(" in your address book!")
                  << std::endl << std::endl;

        const bool list = confirm("Would you like to list everyone in your "
                                  "address book?");

        std::cout << std::endl;

        if (list)
        {
            listAddressBook();
        }
    }
}

void sendFromAddressBook(std::shared_ptr<WalletInfo> walletInfo,
                         uint32_t height, std::string feeAddress)
{
    auto addressBook = getAddressBook();

    if (isAddressBookEmpty(addressBook))
    {
        return;
    }

    std::cout << InformationMsg("Note: You can type cancel at any time to ")
              << InformationMsg("cancel the transaction")
              << std::endl
              << std::endl;

    auto maybeAddressBookEntry = getAddressBookEntry(addressBook);

    if (!maybeAddressBookEntry.isJust)
    {
        std::cout << WarningMsg("Cancelling transaction.") << std::endl;
        return;
    }

    auto maybeAmount = getTransferAmount();

    if (!maybeAmount.isJust)
    {
        std::cout << WarningMsg("Cancelling transction.") << std::endl;
        return;
    }

    auto address = maybeAddressBookEntry.x.address;
    auto amount = maybeAmount.x;
    auto fee = WalletConfig::defaultFee;
    auto extra = getExtraFromPaymentID(maybeAddressBookEntry.x.paymentID);
	auto mixin = WalletConfig::defaultMixin;

    doTransfer(address, amount, fee, extra, walletInfo, height, mixin, feeAddress);
}

bool isAddressBookEmpty(AddressBook addressBook)
{
    if (addressBook.empty())
    {
        std::cout << WarningMsg("Your address book is empty! Add some people ")
                  << WarningMsg("to it first.")
                  << std::endl;

        return true;
    }

    return false;
}

void deleteFromAddressBook()
{
    auto addressBook = getAddressBook();

    if (isAddressBookEmpty(addressBook))
    {
        return;
    }

    while (true)
    {
        std::cout << InformationMsg("Note: You can type cancel at any time ")
                  << InformationMsg("to cancel the deletion")
                  << std::endl
                  << std::endl;

        std::string friendlyName;

        std::cout << InformationMsg("What address book entry do you want to ")
                  << InformationMsg("delete?: ");

        std::getline(std::cin, friendlyName);
        boost::algorithm::trim(friendlyName);

        if (friendlyName == "cancel")
        {
            std::cout << WarningMsg("Cancelling deletion.") << std::endl;
            return;
        }

        auto it = std::find(addressBook.begin(), addressBook.end(),
                            AddressBookEntry(friendlyName));

        if (it != addressBook.end())
        {
            addressBook.erase(it);

            if (saveAddressBook(addressBook))
            {
                std::cout << std::endl
                          << SuccessMsg("This entry has been deleted from ")
                          << SuccessMsg("your address book!")
                          << std::endl;
            }

            return;
        }

        std::cout << std::endl
                  << WarningMsg("Could not find a user with the name of ")
                  << InformationMsg(friendlyName)
                  << WarningMsg(" in your address book!")
                  << std::endl
                  << std::endl;

        bool list = confirm("Would you like to list everyone in your "
                            "address book?");

        std::cout << std::endl;

        if (list)
        {
            listAddressBook();
        }
    }
}

void listAddressBook()
{
    auto addressBook = getAddressBook();

    if (isAddressBookEmpty(addressBook))
    {
        return;
    }

    int index = 1;

    for (const auto &i : addressBook)
    {
        std::cout << InformationMsg("Address Book Entry #")
                  << InformationMsg(std::to_string(index))
                  << InformationMsg(":")
                  << std::endl
                  << std::endl
                  << InformationMsg("Friendly Name: ")
                  << std::endl
                  << SuccessMsg(i.friendlyName)
                  << std::endl
                  << std::endl
                  << InformationMsg("Address: ")
                  << std::endl
                  << SuccessMsg(i.address)
                  << std::endl
                  << std::endl;

        if (i.paymentID != "")
        {
            std::cout << InformationMsg("Payment ID: ")
                      << std::endl
                      << SuccessMsg(i.paymentID)
                      << std::endl
                      << std::endl
                      << std::endl;
        }

        index++;
    }
}

AddressBook getAddressBook()
{
    AddressBook addressBook = boost::value_initialized<decltype(addressBook)>();

    std::ifstream input(WalletConfig::addressBookFilename);

    /* If file exists, read current values */
    if (input)
    {
        std::stringstream buffer;
        buffer << input.rdbuf();
        input.close();

        CryptoNote::loadFromJson(addressBook, buffer.str());
    }

    return addressBook;
}

bool saveAddressBook(AddressBook addressBook)
{
    std::string jsonString = CryptoNote::storeToJson(addressBook);

    std::ofstream output(WalletConfig::addressBookFilename);

    if (output)
    {
        output << jsonString;
    }
    else
    {
        std::cout << WarningMsg("Failed to save address book to disk!")
                  << std::endl
                  << WarningMsg("Check you are able to write files to your ")
                  << WarningMsg("current directory.") << std::endl;

        return false;
    }

    output.close();

    return true;
}
