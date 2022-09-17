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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <emmintrin.h>
#include <wmmintrin.h>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#include "aesb.h"
#include "initializer.h"
#include "Common/int-util.h"
#include "hash-ops.h"
#include "oaes_lib.h"

void (*cn_slow_hash_fp)(void *, const void *, size_t, void *);

void cn_slow_hash_f(void * a, const void * b, size_t c, void * d){
(*cn_slow_hash_fp)(a, b, c, d);
}

#if defined(__GNUC__)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define __attribute__(x)
#endif

#if defined(_MSC_VER)
#define restrict
#endif

#define MEMORY         (1 << 21) /* 2 MiB */
#define ITER           (1 << 20)
#define AES_BLOCK_SIZE  16
#define AES_KEY_SIZE    32 /*16*/
#define INIT_SIZE_BLK   8
#define INIT_SIZE_BYTE (INIT_SIZE_BLK * AES_BLOCK_SIZE)	// 128

#pragma pack(push, 1)
union cn_slow_hash_state {
  union hash_state hs;
  struct {
    uint8_t k[64];
    uint8_t init[INIT_SIZE_BYTE];
  };
};
#pragma pack(pop)

#if defined(_MSC_VER)
#define ALIGNED_DATA(x) __declspec(align(x))
#define ALIGNED_DECL(t, x) ALIGNED_DATA(x) t
#elif defined(__GNUC__)
#define ALIGNED_DATA(x) __attribute__((aligned(x)))
#define ALIGNED_DECL(t, x) t ALIGNED_DATA(x)
#endif

struct cn_ctx {
  ALIGNED_DECL(uint8_t long_state[MEMORY], 16);
  ALIGNED_DECL(union cn_slow_hash_state state, 16);
  ALIGNED_DECL(uint8_t text[INIT_SIZE_BYTE], 16);
  ALIGNED_DECL(uint64_t a[AES_BLOCK_SIZE >> 3], 16);
  ALIGNED_DECL(uint64_t b[AES_BLOCK_SIZE >> 3], 16);
  ALIGNED_DECL(uint8_t c[AES_BLOCK_SIZE], 16);
  oaes_ctx* aes_ctx;
};

static_assert(sizeof(struct cn_ctx) == SLOW_HASH_CONTEXT_SIZE, "Invalid structure size");

static inline void ExpandAESKey256_sub1(__m128i *tmp1, __m128i *tmp2)
{
  __m128i tmp4;
  *tmp2 = _mm_shuffle_epi32(*tmp2, 0xFF);
  tmp4 = _mm_slli_si128(*tmp1, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  *tmp1 = _mm_xor_si128(*tmp1, *tmp2);
}

static inline void ExpandAESKey256_sub2(__m128i *tmp1, __m128i *tmp3)
{
  __m128i tmp2, tmp4;

  tmp4 = _mm_aeskeygenassist_si128(*tmp1, 0x00);
  tmp2 = _mm_shuffle_epi32(tmp4, 0xAA);
  tmp4 = _mm_slli_si128(*tmp3, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  *tmp3 = _mm_xor_si128(*tmp3, tmp2);
}

// Special thanks to Intel for helping me
// with ExpandAESKey256() and its subroutines
static inline void ExpandAESKey256(uint8_t *keybuf)
{
  __m128i tmp1, tmp2, tmp3, *keys;

  keys = (__m128i *)keybuf;

  tmp1 = _mm_load_si128((__m128i *)keybuf);
  tmp3 = _mm_load_si128((__m128i *)(keybuf+0x10));

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x01);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[2] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[3] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x02);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[4] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[5] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x04);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[6] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[7] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x08);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[8] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[9] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x10);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[10] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[11] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x20);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[12] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[13] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x40);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[14] = tmp1;
}

static void (*const extra_hashes[4])(const void *, size_t, char *) =
{
    hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
};

#include "slow-hash.inl"
#define AESNI
#include "slow-hash.inl"

INITIALIZER(detect_aes) {
  int ecx;
#if defined(_MSC_VER)
  int cpuinfo[4];
  __cpuid(cpuinfo, 1);
  ecx = cpuinfo[2];
#else
  int a, b, d;
  __cpuid(1, a, b, ecx, d);
#endif
  cn_slow_hash_fp = (ecx & (1 << 25)) ? &cn_slow_hash_aesni : &cn_slow_hash_noaesni;
}
