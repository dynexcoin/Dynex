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



//////////////////////////////////
#include "Common/FormatTools.h"
//////////////////////////////////

#include <cstdio>
#include <ctime>

#include "../CryptoNoteConfig.h"
#include "CryptoNoteCore/Core.h"
#include "Rpc/CoreRpcServerCommandsDefinitions.h"

namespace Common
{

std::string get_mining_speed(const uint64_t hashrate)
{
    std::stringstream stream;

    stream << std::setprecision(2) << std::fixed;

    if (hashrate > 1e9)
    {
        stream << hashrate / 1e9 << " GH/s";
    }
    else if (hashrate > 1e6)
    {
        stream << hashrate / 1e6 << " MH/s";
    }
    else if (hashrate > 1e3)
    {
        stream << hashrate / 1e3 << " KH/s";
    }
    else
    {
        stream << hashrate << " H/s";
    }

    return stream.str();
}

std::string get_sync_percentage(
    uint64_t height,
    const uint64_t target_height)
{
    /* Don't divide by zero */
    if (height == 0 || target_height == 0)
    {
        return "0.00";
    }

    /* So we don't have > 100% */
    if (height > target_height)
    {
        height = target_height;
    }

    float percent = 100.0f * height / target_height;

    if (height < target_height && percent > 99.99f)
    {
        percent = 99.99f; // to avoid 100% when not fully synced
    }

    std::stringstream stream;

    stream << std::setprecision(2) << std::fixed << percent;

    return stream.str();
}

int numDecimalPlaces = 12;
/* Get the amount we need to divide to convert from atomic to pretty print,
   e.g. 100 for 2 decimal places */
uint64_t getDivisor()
{
    return static_cast<uint64_t>(pow(10, numDecimalPlaces));
}

std::string formatDollars(const uint64_t amount)
{
    /* We want to format our number with comma separators so it's easier to
       use. Now, we could use the nice print_money() function to do this.
       However, whilst this initially looks pretty handy, if we have a locale
       such as ja_JP.utf8, 1 TRTL will actually be formatted as 100 TRTL, which
       is terrible, and could really screw over users.

       So, easy solution right? Just use en_US.utf8! Sure, it's not very
       international, but it'll work! Unfortunately, no. The user has to have
       the locale installed, and if they don't, we get a nasty error at
       runtime.

       Annoyingly, there's no easy way to comma separate numbers outside of
       using the locale method, without writing a pretty long boiler plate
       function. So, instead, we define our own locale, which just returns
       the values we want.
       
       It's less internationally friendly than we would potentially like
       but that would require a ton of scrutinization which if not done could
       land us with quite a few issues and rightfully angry users.
       Furthermore, we'd still have to hack around cases like JP locale
       formatting things incorrectly, and it makes reading in inputs harder
       too. */

    /* Thanks to https://stackoverflow.com/a/7277333/8737306 for this neat
       workaround */
    class comma_numpunct : public std::numpunct<char>
    {
        protected:
            virtual char do_thousands_sep() const
            {
                return ',';
            }

            virtual std::string do_grouping() const
            {
                return "\03";
            }
    };

    std::locale comma_locale(std::locale(), new comma_numpunct());
    std::stringstream stream;
    stream.imbue(comma_locale);
    stream << amount;
    return stream.str();
}

/* Pad to the amount of decimal spaces, e.g. with 2 decimal spaces 5 becomes
   05, 50 remains 50 */
std::string formatCents(const uint64_t amount)
{
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(numDecimalPlaces)
           << amount;
    return stream.str();
}

std::string formatAmount(const uint64_t amount)
{
    const uint64_t divisor = getDivisor();
    const uint64_t dollars = amount / divisor;
    const uint64_t cents = amount % divisor;

    return formatDollars(dollars) + "." + formatCents(cents) + " "
         + "CROAT";
}

std::string formatAmountBasic(const uint64_t amount)
{
    const uint64_t divisor = getDivisor();
    const uint64_t dollars = amount / divisor;
    const uint64_t cents = amount % divisor;

    return std::to_string(dollars) + "." + formatCents(cents);
}

std::string prettyPrintBytes(uint64_t input)
{
    /* Store as a double so we can have 12.34 kb for example */
    double numBytes = static_cast<double>(input);

    std::vector<std::string> suffixes = { "B", "KB", "MB", "GB", "TB"};

    uint64_t selectedSuffix = 0;

    while (numBytes >= 1024 && selectedSuffix < suffixes.size() - 1)
    {
        selectedSuffix++;

        numBytes /= 1024;
    }

    std::stringstream msg;

    msg << std::fixed << std::setprecision(2) << numBytes << " "
        << suffixes[selectedSuffix];

    return msg.str();
}

std::string unixTimeToDate(const uint64_t timestamp)
{
    const std::time_t time = timestamp;
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%F %R", std::localtime(&time));
    return std::string(buffer);
}

} // namespace Common
