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

#include "Process.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace Tests {

  void Process::startChild(const std::string& executablePath, const std::vector<std::string>& args) {
   
#if defined WIN32
    std::stringstream ss;
    ss << "start /MIN " << executablePath;

    for (const auto& arg: args) {
      ss << " \"" << arg << "\"";
    }

    auto cmdline = ss.str();
    system(cmdline.c_str());

#else
    std::vector<const char*> cargs;
    cargs.push_back(executablePath.c_str());
    for (const auto& arg : args) {
      cargs.push_back(arg.c_str());
    }

    cargs.push_back(nullptr);

    auto pid = fork();

    if (pid == 0) {
      if (execv(executablePath.c_str(), (char**)&cargs[0]) == -1) {
        printf("Failed to start %s: %d\n", executablePath.c_str(), errno);
        exit(404);
      }
    } else if (pid > 0) {
      m_pid = pid;
    } else if (pid < 0) {
      throw std::runtime_error("fork() failed");
    }
#endif

  }

  void Process::wait() {
#ifndef _WIN32
    if (m_pid == 0) {
      return;
    }

    int status;
    waitpid(m_pid, &status, 0);
    m_pid = 0;
#endif
  }

}
