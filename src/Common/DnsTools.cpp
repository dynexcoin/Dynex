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


#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <functional>
#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <boost/program_options/variables_map.hpp>
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
//#include <crtdbg.h>
//#include <winsock2.h>
#include <windns.h>
#include <Rpc.h>
#else
#include <arpa/nameser.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include "DnsTools.h"

namespace Common {

#ifndef __ANDROID__

	bool fetch_dns_txt(const std::string domain, std::vector<std::string>&records) {

#ifdef _WIN32
		using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")

		PDNS_RECORD pDnsRecord;          //Pointer to DNS_RECORD structure.

		{
			WORD type = DNS_TYPE_TEXT;

			if (0 != DnsQuery_A(domain.c_str(), type, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL))
			{
				cerr << "Error querying: '" << domain << "'" << endl;
				return false;
			}
		}

		PDNS_RECORD it;
		map<WORD, function<void(void)>> callbacks;

		callbacks[DNS_TYPE_TEXT] = [&it, &records](void) -> void {
			std::stringstream stream;
			for (DWORD i = 0; i < it->Data.TXT.dwStringCount; i++) {
				stream << RPC_CSTR(it->Data.TXT.pStringArray[i]) << endl;;
			}
			records.push_back(stream.str());
		};

		for (it = pDnsRecord; it != NULL; it = it->pNext) {
			if (callbacks.count(it->wType)) {
				callbacks[it->wType]();
			}
		}
		DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);
#else
		using namespace std;

		res_init();
		ns_msg nsMsg;
		int response;
		unsigned char query_buffer[4096];
		{
			ns_type type = ns_t_txt;

			const char * c_domain = (domain).c_str();
			response = res_query(c_domain, 1, type, query_buffer, sizeof(query_buffer));

			if (response < 0)
				return false;
		}

		ns_initparse(query_buffer, response, &nsMsg);

		map<ns_type, function<void(const ns_rr &rr)>> callbacks;

		callbacks[ns_t_txt] = [&nsMsg, &records](const ns_rr &rr) -> void {
			int txt_len = *(unsigned char *) ns_rr_rdata(rr);
			char txt[256];
			memset(txt, 0, 256);
			if (txt_len <= 255){
				memcpy(txt, ns_rr_rdata(rr) + 1, txt_len);
				records.push_back(txt);
			}
		};

		for (int x = 0; x < ns_msg_count(nsMsg, ns_s_an); x++) {
			ns_rr rr;
			ns_parserr(&nsMsg, ns_s_an, x, &rr);
			ns_type type = ns_rr_type(rr);
			if (callbacks.count(type)) {
				callbacks[type](rr);
			}
		}

#endif
		if (records.empty())
			return false;

		return true;
	}

#endif

}