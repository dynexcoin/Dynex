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

#include <cstddef>
#include <initializer_list>

namespace CryptoNote {

struct CheckpointData {
  uint32_t height;
  const char* blockId;
};

const std::initializer_list<CheckpointData> CHECKPOINTS = {
  {0,      "d0b259e5dad976692f3b41f18f2d427a53b3c2f96a5872ac48ece02998296a33"},
  {10000,  "663ce89e518149d1daa346ea71fb582ac7628f4940ba29a9edf77f40ec707744"},
  {20000,  "d3536752671b20351d5adc639a7d8abf1ccbc5ec6b619889aa23fd8583ae4744"},
  {24405,  "48fd9907a4e5f4a6b8d0ff026ea5cc48e7e656f612210964eb9583ddb508ac32"},
  {24316,  "9d58ea1c9257a3dff39b39e06e002a55700af4ce6f1498006866b5dc0c4005a4"},
  {24585,  "292e83cba1d98e8ecb2f0f53abc7a4a444b4145da3645a2746dad1e82e2e058a"},
  {24771,  "9e767b96366dfa0bf405e79ce6b2b0c7534f6e58bfffde69e572a4c8e4df589c"},
  {35000,  "6299ac6928e6510ebc4ed4be3b604ab83493e8fe18b2c61580026f9c0fc55ccb"},
  {45000,  "9ef1c10bdbfffea890123e5a6b24c99d7e91937cdf610557ae387baf86da761b"},
  {58300,  "98d37a7c9bf0d64d6d4bde39227ab760fe4d233eb3d884bf0ce748d556d20799"},
  {58386,  "cab2f1b6c898354fd1837f1d92e5299c9302a7b71d53739f57edb656dcc843cb"},
  {58494,  "ff99c288f490d141d6b5877b708352a682205f333d46bb618697d66b3344ef99"},
  {58505,  "4c8d143eda89c2f83f87e744bb69474f4c729a3fd9d986556fcadc5d3bffb8ef"},
  {68602,  "d3ad020d97089c80deaccc0c2ae0579e4e5804324f6f9a4231cd150bcdce2be4"},
  {68603,  "a5b52bc2f7ceb69fcc8c692aa76e266a862a8b7d9a070f3c5790bdf0356fd43c"},
  {70336,  "f784fe160687d45c274e9d5ce91e45f5d2febece5ab54d2d235a2bfc6fc826df"},
  {70348,  "69580769b4e2f794cf9476d3192529ce270fe3303d68050bbd190d8e2d6ca71e"},
  {70404,  "e1c5cd42b496cdd7369c6a36b465d09fe96b3a33090c3b99f5a7570279badc37"},
  {108000, "30a2ccb7c7ddbb10283538b221d35a2f5720b2b874330588cda4c906c2104776"},
  {112000, "b0d59a416ed0fb9ad16ac9ba035abd32f69d2f8d59ea64361a4c7421abf5f1d0"}
};

}
