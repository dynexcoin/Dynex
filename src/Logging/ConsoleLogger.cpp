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


#include "ConsoleLogger.h"
#include <iostream>
#include <unordered_map>
#include <Common/ConsoleTools.h>


namespace Logging {

using Common::Console::Color;

ConsoleLogger::ConsoleLogger(Level level) : CommonLogger(level) {
}

void ConsoleLogger::doLogString(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex);
  bool readingText = true;
  bool changedColor = false;
  std::string color = "";

  static std::unordered_map<std::string, Color> colorMapping = {
    { BLUE, Color::Blue },
    { GREEN, Color::Green },
    { RED, Color::Red },
    { YELLOW, Color::Yellow },
    { WHITE, Color::White },
    { CYAN, Color::Cyan },
    { MAGENTA, Color::Magenta },

    { BRIGHT_BLUE, Color::BrightBlue },
    { BRIGHT_GREEN, Color::BrightGreen },
    { BRIGHT_RED, Color::BrightRed },
    { BRIGHT_YELLOW, Color::BrightYellow },
    { BRIGHT_WHITE, Color::BrightWhite },
    { BRIGHT_CYAN, Color::BrightCyan },
    { BRIGHT_MAGENTA, Color::BrightMagenta },

    { DEFAULT, Color::Default }
  };

  for (size_t charPos = 0; charPos < message.size(); ++charPos) {
    if (message[charPos] == ILogger::COLOR_DELIMETER) {
      readingText = !readingText;
      color += message[charPos];
      if (readingText) {
        auto it = colorMapping.find(color);
        Common::Console::setTextColor(it == colorMapping.end() ? Color::Default : it->second);
        changedColor = true;
        color.clear();
      }
    } else if (readingText) {
      std::cout << message[charPos];
    } else {
      color += message[charPos];
    }
  }

  if (changedColor) {
    Common::Console::setTextColor(Color::Default);
  }
}

}
