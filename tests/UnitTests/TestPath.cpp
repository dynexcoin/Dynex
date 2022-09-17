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

#include "gtest/gtest.h"
#include "Common/PathTools.h"

TEST(PathTools, NativePathToGeneric) {

#ifdef _WIN32
  const std::string input = "C:\\Windows\\System\\etc\\file.exe";
  const std::string output = "C:/Windows/System/etc/file.exe";
#else
  const std::string input = "/var/tmp/file.tmp";
  const std::string output = input;

#endif

  auto path = Common::NativePathToGeneric(input);
  ASSERT_EQ(output, path);
}

TEST(PathTools, GetExtension) {
  ASSERT_EQ("", Common::GetExtension(""));
  ASSERT_EQ(".ext", Common::GetExtension(".ext"));

  ASSERT_EQ("", Common::GetExtension("test"));
  ASSERT_EQ(".ext", Common::GetExtension("test.ext"));
  ASSERT_EQ(".ext2", Common::GetExtension("test.ext.ext2"));

  ASSERT_EQ(".ext", Common::GetExtension("/path/file.ext"));
  ASSERT_EQ(".yyy", Common::GetExtension("/path.xxx/file.yyy"));
  ASSERT_EQ("", Common::GetExtension("/path.ext/file"));
}

TEST(PathTools, RemoveExtension) {

  ASSERT_EQ("", Common::RemoveExtension(""));
  ASSERT_EQ("", Common::RemoveExtension(".ext"));

  ASSERT_EQ("test", Common::RemoveExtension("test"));
  ASSERT_EQ("test", Common::RemoveExtension("test.ext"));
  ASSERT_EQ("test.ext", Common::RemoveExtension("test.ext.ext2"));

  ASSERT_EQ("/path/file", Common::RemoveExtension("/path/file.ext"));
  ASSERT_EQ("/path.ext/file", Common::RemoveExtension("/path.ext/file.ext"));
  ASSERT_EQ("/path.ext/file", Common::RemoveExtension("/path.ext/file"));
}

TEST(PathTools, SplitPath) {
  std::string dir;
  std::string file;

  Common::SplitPath("/path/more/file", dir, file);

  ASSERT_EQ("/path/more", dir);
  ASSERT_EQ("file", file);

  Common::SplitPath("file.ext", dir, file);

  ASSERT_EQ("", dir);
  ASSERT_EQ("file.ext", file);

  Common::SplitPath("/path/more/", dir, file);

  ASSERT_EQ("/path/more", dir);
  ASSERT_EQ("", file);
}
