// Copyright (c) 2021-2023, Dynex Developers
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
// Copyright (c) 2012-2016, The CN developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#ifndef __FreeBSD__
#include <alloca.h>
#endif
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#ifdef __FreeBSD__
#define alloca(x)  __builtin_alloca(x)
#endif

#include "hash-ops.h"

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash) {
  assert(count > 0);
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
  } else if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
  } else {
    size_t i, j;
    size_t cnt = count - 1;
    for (i = 1; i < 8 * sizeof(size_t); i <<= 1) {
      cnt |= cnt >> i;
    }
    cnt &= ~(cnt >> 1);
    char *ints = calloc(cnt, HASH_SIZE);
    assert(ints);
    memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);
    for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
      cn_fast_hash(hashes[i], 2 * HASH_SIZE, ints + j * HASH_SIZE);
    }
    assert(i == count);
    while (cnt > 2) {
      cnt >>= 1;
      for (i = 0, j = 0; j < cnt; i += 2, ++j) {
        cn_fast_hash(ints + i * HASH_SIZE, 2 * HASH_SIZE, ints + j * HASH_SIZE);
      }
    }
    cn_fast_hash(ints, 2 * HASH_SIZE, root_hash);
    free(ints);
  }
}

size_t tree_depth(size_t count) {
  size_t i;
  size_t depth = 0;
  assert(count > 0);
  for (i = sizeof(size_t) << 2; i > 0; i >>= 1) {
    if (count >> i > 0) {
      count >>= i;
      depth += i;
    }
  }
  return depth;
}

void tree_branch(const char (*hashes)[HASH_SIZE], size_t count, char (*branch)[HASH_SIZE]) {
  size_t i, j;
  size_t cnt = 1;
  size_t depth = 0;
  char (*ints)[HASH_SIZE];
  assert(count > 0);
  for (i = sizeof(size_t) << 2; i > 0; i >>= 1) {
    if (cnt << i <= count) {
      cnt <<= i;
      depth += i;
    }
  }
  assert(cnt == 1ULL << depth);
  assert(depth == tree_depth(count));
  ints = alloca((cnt - 1) * HASH_SIZE);
  memcpy(ints, hashes + 1, (2 * cnt - count - 1) * HASH_SIZE);
  for (i = 2 * cnt - count, j = 2 * cnt - count - 1; j < cnt - 1; i += 2, ++j) {
    cn_fast_hash(hashes[i], 2 * HASH_SIZE, ints[j]);
  }
  assert(i == count);
  while (depth > 0) {
    assert(cnt == 1ULL << depth);
    cnt >>= 1;
    --depth;
    memcpy(branch[depth], ints[0], HASH_SIZE);
    for (i = 1, j = 0; j < cnt - 1; i += 2, ++j) {
      cn_fast_hash(ints[i], 2 * HASH_SIZE, ints[j]);
    }
  }
}

void tree_hash_from_branch(const char (*branch)[HASH_SIZE], size_t depth, const char *leaf, const void *path, char *root_hash) {
  if (depth == 0) {
    memcpy(root_hash, leaf, HASH_SIZE);
  } else {
    char buffer[2][HASH_SIZE];
    int from_leaf = 1;
    char *leaf_path, *branch_path;
    while (depth > 0) {
      --depth;
      if (path && (((const char *) path)[depth >> 3] & (1 << (depth & 7))) != 0) {
        leaf_path = buffer[1];
        branch_path = buffer[0];
      } else {
        leaf_path = buffer[0];
        branch_path = buffer[1];
      }
      if (from_leaf) {
        memcpy(leaf_path, leaf, HASH_SIZE);
        from_leaf = 0;
      } else {
        cn_fast_hash(buffer, 2 * HASH_SIZE, leaf_path);
      }
      memcpy(branch_path, branch[depth], HASH_SIZE);
    }
    cn_fast_hash(buffer, 2 * HASH_SIZE, root_hash);
  }
}
