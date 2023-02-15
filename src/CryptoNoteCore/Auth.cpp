// ----------------------------------------------------------------------------------------------------
// DYNEX
// ----------------------------------------------------------------------------------------------------
// Copyright (c) 2021-2022, The Dynex Project
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

#include "Auth.h"

#include <string>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <curl/curl.h>

#include <Common/JsonValue.h>

#ifdef WIN32
	#undef ERROR // windows.h
	#define __builtin_bswap32(x) _byteswap_ulong(x)
#endif // WIN32

using namespace Common;
using namespace Logging;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace CryptoNote {

// curl return value function
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool AuthBlock(uint32_t height, uint32_t nonce, ILogger& log) {
	static LoggerRef logger(log, "mallob");

	std::stringstream ss; ss << std::hex << std::setfill('0') << std::setw(8) << __builtin_bswap32(nonce);
	std::string url = mallob_endpoint + "/api/v2/node?method=verify_block&height=" + std::to_string(height) + "&nonce=" + ss.str();
	logger(DEBUGGING) << "Mallob request: " << url;

	CURLcode res;
	CURL *curl = curl_easy_init();
	struct curl_slist *list = NULL;
	std::string readBuffer;
	list = curl_slist_append(list, "Accept: application/json");
	list = curl_slist_append(list, "Content-type: application/json");
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // ?
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // ?
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res != CURLE_OK) {
			logger(ERROR) << "Mallob curl error: " << curl_easy_strerror(res);
		} else {
			logger(DEBUGGING) << "Mallob answer: " <<  readBuffer;
			try {
				std::stringstream stream(readBuffer);
				JsonValue json;
				stream >> json;
				if (json.contains("status")) {
					return json("status").getBool();
				}
			}
			catch(const std::exception &e) {
				logger(ERROR) << "Mallob json parse error: " << readBuffer;
			}
		}
	}
	return false;
}

}
