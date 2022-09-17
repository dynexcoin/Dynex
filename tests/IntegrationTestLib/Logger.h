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

#pragma once

#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

#ifdef LOG_WARNING
#undef LOG_WARNING
#endif

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <map>
#include <mutex>

#ifdef _WIN32
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#else
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#endif

#define LOG_(str , lvl , idnt) (CLogger::Instance().Log((std::string("")+(str)), (lvl), (idnt)))
#define LOG_VERBOSE(str) LOG_((str), (CLogger::VERBOSE),0 )
#define LOG_TRACE(str) LOG_((str), (CLogger::TRACE),0 )
#define LOG_DEBUG(str) LOG_((str), (CLogger::DEBUG),0 )
#define LOG_ERROR(str) LOG_((str), (CLogger::_ERROR), 0 )
#define LOG_WARNING(str) LOG_((str), (CLogger::WARNING),0 )

#define TO_STRING(param) boost::lexical_cast<std::string>((param))


class CLogger
{
public:
	enum LOG_LEVEL
	{
		VERBOSE,
		DEBUG,
		TRACE,
		WARNING,
		_ERROR
	};
	static CLogger& Instance();
  void init(LOG_LEVEL log_lvl);
	void Log(const std::string & log_info, LOG_LEVEL log_lvl, int indent_inc=0);

private: 
	int indent;
	std::map<LOG_LEVEL, std::string> level_names;
	LOG_LEVEL log_level;
	std::mutex mutex;
	CLogger(){};
	CLogger(const CLogger& root);
	CLogger& operator=(const CLogger&);
};
