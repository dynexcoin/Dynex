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

#include <Common/ConsoleTools.h>

#include <iomanip>

#include <ostream>

#include <string>

class ColouredMsg
{
    public:
        ColouredMsg(std::string msg, Common::Console::Color colour) 
                  : msg(msg), colour(colour) {}

        ColouredMsg(std::string msg, int padding, 
                    Common::Console::Color colour)
                  : msg(msg), colour(colour), padding(padding), pad(true) {}


        /* Set the text colour, write the message, then reset. We use a class
           as it seems the only way to have a valid << operator. We need this
           so we can nicely do something like:

           std::cout << "Hello " << GreenMsg("user") << std::endl;

           Without having to write:

           std::cout << "Hello ";
           GreenMsg("user");
           std::cout << std::endl; */

        friend std::ostream& operator<<(std::ostream& os, const ColouredMsg &m)
        {
            Common::Console::setTextColor(m.colour);

            if (m.pad)
            {
                os << std::left << std::setw(m.padding) << m.msg;
            }
            else
            {
                os << m.msg;
            }

            Common::Console::setTextColor(Common::Console::Color::Default);
            return os;
        }

    protected:
        std::string msg;
        const Common::Console::Color colour;
        const int padding = 0;
        const bool pad = false;
};

class SuccessMsg : public ColouredMsg
{
    public:
        explicit SuccessMsg(std::string msg) 
               : ColouredMsg(msg, Common::Console::Color::Green) {}

        explicit SuccessMsg(std::string msg, int padding)
               : ColouredMsg(msg, padding, Common::Console::Color::Green) {}
};

class InformationMsg : public ColouredMsg
{
    public:
        explicit InformationMsg(std::string msg) 
               : ColouredMsg(msg, Common::Console::Color::BrightYellow) {}

        explicit InformationMsg(std::string msg, int padding)
               : ColouredMsg(msg, padding, 
                             Common::Console::Color::BrightYellow) {}
};

class SuggestionMsg : public ColouredMsg
{
    public:
        explicit SuggestionMsg(std::string msg) 
               : ColouredMsg(msg, Common::Console::Color::BrightBlue) {}

        explicit SuggestionMsg(std::string msg, int padding)
               : ColouredMsg(msg, padding, 
                             Common::Console::Color::BrightBlue) {}
};

class WarningMsg : public ColouredMsg
{
    public:
        explicit WarningMsg(std::string msg) 
               : ColouredMsg(msg, Common::Console::Color::BrightRed) {}

        explicit WarningMsg(std::string msg, int padding)
               : ColouredMsg(msg, padding, 
                             Common::Console::Color::BrightRed) {}
};
