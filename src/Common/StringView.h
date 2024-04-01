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


#pragma once

#include <cassert>
#include <string>

namespace Common {

// 'StringView' is a pair of pointer to constant char and size.
// It is recommended to pass 'StringView' to procedures by value.
// 'StringView' supports 'EMPTY' and 'NIL' representations as follows:
//   'data' == 'nullptr' && 'size' == 0 - EMPTY NIL
//   'data' != 'nullptr' && 'size' == 0 - EMPTY NOTNIL
//   'data' == 'nullptr' && 'size' > 0 - Undefined
//   'data' != 'nullptr' && 'size' > 0 - NOTEMPTY NOTNIL
class StringView {
public:
  typedef char Object;
  typedef size_t Size;

  const static Size INVALID;
  const static StringView EMPTY;
  const static StringView NIL;

  // Default constructor.
  // Leaves object uninitialized. Any usage before initializing it is undefined.
  StringView();

  // Direct constructor.
  // The behavior is undefined unless 'stringData' != 'nullptr' || 'stringSize' == 0
  StringView(const Object* stringData, Size stringSize);

  // Constructor from C array.
  // The behavior is undefined unless 'stringData' != 'nullptr' || 'stringSize' == 0. Input state can be malformed using poiner conversions.
  template<Size stringSize> StringView(const Object(&stringData)[stringSize]) : data(stringData), size(stringSize - 1) {
    assert(data != nullptr || size == 0);
  }

  // Constructor from std::string
  StringView(const std::string& string);

  // Copy constructor.
  // Performs default action - bitwise copying of source object.
  // The behavior is undefined unless 'other' 'StringView' is in defined state, that is 'data' != 'nullptr' || 'size' == 0
  StringView(const StringView& other);

  // Destructor.
  // No special action is performed.
  ~StringView();

  // Copy assignment operator.
  // The behavior is undefined unless 'other' 'StringView' is in defined state, that is 'data' != 'nullptr' || 'size' == 0
  StringView& operator=(const StringView& other);

  explicit operator std::string() const;

  const Object* getData() const;

  Size getSize() const;

  // Return false if 'StringView' is not EMPTY.
  // The behavior is undefined unless 'StringView' was initialized.
  bool isEmpty() const;

  // Return false if 'StringView' is not NIL.
  // The behavior is undefined unless 'StringView' was initialized.
  bool isNil() const;

  // Get 'StringView' element by index.
  // The behavior is undefined unless 'StringView' was initialized and 'index' < 'size'.
  const Object& operator[](Size index) const;

  // Get first element.
  // The behavior is undefined unless 'StringView' was initialized and 'size' > 0
  const Object& first() const;

  // Get last element.
  // The behavior is undefined unless 'StringView' was initialized and 'size' > 0
  const Object& last() const;

  // Return a pointer to the first element.
  // The behavior is undefined unless 'StringView' was initialized.
  const Object* begin() const;

  // Return a pointer after the last element.
  // The behavior is undefined unless 'StringView' was initialized.
  const Object* end() const;

  // Compare elements of two strings, return false if there is a difference.
  // EMPTY and NIL strings are considered equal.
  // The behavior is undefined unless both strings were initialized.
  bool operator==(StringView other) const;

  // Compare elements two strings, return false if there is no difference.
  // EMPTY and NIL strings are considered equal.
  // The behavior is undefined unless both strings were initialized.
  bool operator!=(StringView other) const;

  // Compare two strings character-wise.
  // The behavior is undefined unless both strings were initialized.
  bool operator<(StringView other) const;

  // Compare two strings character-wise.
  // The behavior is undefined unless both strings were initialized.
  bool operator<=(StringView other) const;

  // Compare two strings character-wise.
  // The behavior is undefined unless both strings were initialized.
  bool operator>(StringView other) const;

  // Compare two strings character-wise.
  // The behavior is undefined unless both strings were initialized.
  bool operator>=(StringView other) const;

  // Return false if 'StringView' does not contain 'object' at the beginning.
  // The behavior is undefined unless 'StringView' was initialized.
  bool beginsWith(const Object& object) const;

  // Return false if 'StringView' does not contain 'other' at the beginning.
  // The behavior is undefined unless both strings were initialized.
  bool beginsWith(StringView other) const;

  // Return false if 'StringView' does not contain 'object'.
  // The behavior is undefined unless 'StringView' was initialized.
  bool contains(const Object& object) const;

  // Return false if 'StringView' does not contain 'other'.
  // The behavior is undefined unless both strings were initialized.
  bool contains(StringView other) const;

  // Return false if 'StringView' does not contain 'object' at the end.
  // The behavior is undefined unless 'StringView' was initialized.
  bool endsWith(const Object& object) const;

  // Return false if 'StringView' does not contain 'other' at the end.
  // The behavior is undefined unless both strings were initialized.
  bool endsWith(StringView other) const;

  // Looks for the first occurence of 'object' in 'StringView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless 'StringView' was initialized.
  Size find(const Object& object) const;

  // Looks for the first occurence of 'other' in 'StringView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless both strings were initialized.
  Size find(StringView other) const;

  // Looks for the last occurence of 'object' in 'StringView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless 'StringView' was initialized.
  Size findLast(const Object& object) const;

  // Looks for the first occurence of 'other' in 'StringView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless both strings were initialized.
  Size findLast(StringView other) const;

  // Returns substring of 'headSize' first elements.
  // The behavior is undefined unless 'StringView' was initialized and 'headSize' <= 'size'.
  StringView head(Size headSize) const;

  // Returns substring of 'tailSize' last elements.
  // The behavior is undefined unless 'StringView' was initialized and 'tailSize' <= 'size'.
  StringView tail(Size tailSize) const;

  // Returns 'StringView' without 'headSize' first elements.
  // The behavior is undefined unless 'StringView' was initialized and 'headSize' <= 'size'.
  StringView unhead(Size headSize) const;

  // Returns 'StringView' without 'tailSize' last elements.
  // The behavior is undefined unless 'StringView' was initialized and 'tailSize' <= 'size'.
  StringView untail(Size tailSize) const;

  // Returns substring starting at 'startIndex' and contaning 'endIndex' - 'startIndex' elements.
  // The behavior is undefined unless 'StringView' was initialized and 'startIndex' <= 'endIndex' and 'endIndex' <= 'size'.
  StringView range(Size startIndex, Size endIndex) const;

  // Returns substring starting at 'startIndex' and contaning 'sliceSize' elements.
  // The behavior is undefined unless 'StringView' was initialized and 'startIndex' <= 'size' and 'startIndex' + 'sliceSize' <= 'size'.
  StringView slice(Size startIndex, Size sliceSize) const;

protected:
  const Object* data;
  Size size;
};

}
