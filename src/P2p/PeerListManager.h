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

#include <list>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include "P2pProtocolTypes.h"
#include "CryptoNoteConfig.h"

namespace CryptoNote {

class ISerializer;
/************************************************************************/
/*                                                                      */
/************************************************************************/
class PeerlistManager {
  struct by_time{};
  struct by_id{};
  struct by_addr{};

  typedef boost::multi_index_container<
    PeerlistEntry,
    boost::multi_index::indexed_by<
    // access by peerlist_entry::net_adress
    boost::multi_index::ordered_unique<boost::multi_index::tag<by_addr>, boost::multi_index::member<PeerlistEntry, NetworkAddress, &PeerlistEntry::adr> >,
    // sort by peerlist_entry::last_seen<
    boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_time>, boost::multi_index::member<PeerlistEntry, uint64_t, &PeerlistEntry::last_seen> >
    >
  > peers_indexed;

public:

  class Peerlist {
  public:
    Peerlist(peers_indexed& peers, size_t maxSize);
    size_t count() const;
    bool get(PeerlistEntry& entry, size_t index) const;
    void trim();

  private:
    peers_indexed& m_peers;
    const size_t m_maxSize;
  };

  PeerlistManager();

  bool init(bool allow_local_ip);
  size_t get_white_peers_count() const { return m_peers_white.size(); }
  size_t get_gray_peers_count() const { return m_peers_gray.size(); }
  bool merge_peerlist(const std::list<PeerlistEntry>& outer_bs);
  bool get_peerlist_head(std::list<PeerlistEntry>& bs_head, uint32_t depth = CryptoNote::P2P_DEFAULT_PEERS_IN_HANDSHAKE) const;
  bool get_peerlist_full(std::list<PeerlistEntry>& pl_gray, std::list<PeerlistEntry>& pl_white) const;
  bool get_white_peer_by_index(PeerlistEntry& p, size_t i) const;
  bool get_gray_peer_by_index(PeerlistEntry& p, size_t i) const;
  bool append_with_peer_white(const PeerlistEntry& pr);
  bool append_with_peer_gray(const PeerlistEntry& pr);
  bool set_peer_just_seen(PeerIdType peer, uint32_t ip, uint32_t port);
  bool set_peer_just_seen(PeerIdType peer, const NetworkAddress& addr);
  bool set_peer_unreachable(const PeerlistEntry& pr);
  bool is_ip_allowed(uint32_t ip) const;
  void trim_white_peerlist();
  void trim_gray_peerlist();

  void serialize(ISerializer& s);

  Peerlist& getWhite();
  Peerlist& getGray();

private:
  std::string m_config_folder;
  bool m_allow_local_ip;
  peers_indexed m_peers_gray;
  peers_indexed m_peers_white;
  Peerlist m_whitePeerlist;
  Peerlist m_grayPeerlist;
};

}
