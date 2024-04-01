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
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Common {

// 'ArrayView' is a pair of pointer to constant object of parametrized type and size.
// It is recommended to pass 'ArrayView' to procedures by value.
// 'ArrayView' supports 'EMPTY' and 'NIL' representations as follows:
//   'data' == 'nullptr' && 'size' == 0 - EMPTY NIL
//   'data' != 'nullptr' && 'size' == 0 - EMPTY NOTNIL
//   'data' == 'nullptr' && 'size' > 0 - Undefined
//   'data' != 'nullptr' && 'size' > 0 - NOTEMPTY NOTNIL
// For signed integer 'Size', 'ArrayView' with 'size' < 0 is undefined.
template<class Object = uint8_t, class Size = size_t> class ArrayView {
public:
  typedef Object ObjectType;
  typedef Size SizeType;

  const static Size INVALID;
  const static ArrayView EMPTY;
  const static ArrayView NIL;

  // Default constructor.
  // Leaves object uninitialized. Any usage before initializing it is undefined.
  ArrayView()
#ifndef NDEBUG
    : data(nullptr), size(INVALID) // In debug mode, fill in object with invalid state (undefined).
#endif
  {
  }

  // Direct constructor.
  // The behavior is undefined unless 'arrayData' != 'nullptr' || 'arraySize' == 0
  ArrayView(const Object* arrayData, Size arraySize) : data(arrayData), size(arraySize) {
    assert(data != nullptr || size == 0);
  }

  // Constructor from C array.
  // The behavior is undefined unless 'arrayData' != 'nullptr' || 'arraySize' == 0. Input state can be malformed using poiner conversions.
  template<Size arraySize> ArrayView(const Object(&arrayData)[arraySize]) : data(arrayData), size(arraySize) {
    assert(data != nullptr || size == 0);
  }

  // Copy constructor.
  // Performs default action - bitwise copying of source object.
  // The behavior is undefined unless 'other' 'ArrayView' is in defined state, that is 'data' != 'nullptr' || 'size' == 0
  ArrayView(const ArrayView& other) : data(other.data), size(other.size) {
    assert(data != nullptr || size == 0);
  }

  // Destructor.
  // No special action is performed.
  ~ArrayView() {
  }

  // Copy assignment operator.
  // The behavior is undefined unless 'other' 'ArrayView' is in defined state, that is 'data' != 'nullptr' || 'size' == 0
  ArrayView& operator=(const ArrayView& other) {
    assert(other.data != nullptr || other.size == 0);
    data = other.data;
    size = other.size;
    return *this;
  }

  const Object* getData() const {
    assert(data != nullptr || size == 0);
    return data;
  }

  Size getSize() const {
    assert(data != nullptr || size == 0);
    return size;
  }

  // Return false if 'ArrayView' is not EMPTY.
  // The behavior is undefined unless 'ArrayView' was initialized.
  bool isEmpty() const {
    assert(data != nullptr || size == 0);
    return size == 0;
  }

  // Return false if 'ArrayView' is not NIL.
  // The behavior is undefined unless 'ArrayView' was initialized.
  bool isNil() const {
    assert(data != nullptr || size == 0);
    return data == nullptr;
  }

  // Get 'ArrayView' element by index.
  // The behavior is undefined unless 'ArrayView' was initialized and 'index' < 'size'.
  const Object& operator[](Size index) const {
    assert(data != nullptr || size == 0);
    assert(index < size);
    return *(data + index);
  }

  // Get first element.
  // The behavior is undefined unless 'ArrayView' was initialized and 'size' > 0
  const Object& first() const {
    assert(data != nullptr || size == 0);
    assert(size > 0);
    return *data;
  }

  // Get last element.
  // The behavior is undefined unless 'ArrayView' was initialized and 'size' > 0
  const Object& last() const {
    assert(data != nullptr || size == 0);
    assert(size > 0);
    return *(data + (size - 1));
  }

  // Return a pointer to the first element.
  // The behavior is undefined unless 'ArrayView' was initialized.
  const Object* begin() const {
    assert(data != nullptr || size == 0);
    return data;
  }

  // Return a pointer after the last element.
  // The behavior is undefined unless 'ArrayView' was initialized.
  const Object* end() const {
    assert(data != nullptr || size == 0);
    return data + size;
  }

  // Compare elements of two arrays, return false if there is a difference.
  // EMPTY and NIL arrays are considered equal.
  // The behavior is undefined unless both arrays were initialized.
  bool operator==(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size == other.size) {
      for (Size i = 0;; ++i) {
        if (i == size) {
          return true;
        }

        if (!(*(data + i) == *(other.data + i))) {
          break;
        }
      }
    }

    return false;
  }

  // Compare elements two arrays, return false if there is no difference.
  // EMPTY and NIL arrays are considered equal.
  // The behavior is undefined unless both arrays were initialized.
  bool operator!=(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size == other.size) {
      for (Size i = 0;; ++i) {
        if (i == size) {
          return false;
        }

        if (*(data + i) != *(other.data + i)) {
          break;
        }
      }
    }

    return true;
  }

  // Return false if 'ArrayView' does not contain 'object' at the beginning.
  // The behavior is undefined unless 'ArrayView' was initialized.
  bool beginsWith(const Object& object) const {
    assert(data != nullptr || size == 0);
    if (size == 0) {
      return false;
    }

    return *data == object;
  }

  // Return false if 'ArrayView' does not contain 'other' at the beginning.
  // The behavior is undefined unless both arrays were initialized.
  bool beginsWith(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size >= other.size) {
      for (Size i = 0;; ++i) {
        if (i == other.size) {
          return true;
        }

        if (!(*(data + i) == *(other.data + i))) {
          break;
        }
      }
    }

    return false;
  }

  // Return false if 'ArrayView' does not contain 'object'.
  // The behavior is undefined unless 'ArrayView' was initialized.
  bool contains(const Object& object) const {
    assert(data != nullptr || size == 0);
    for (Size i = 0; i < size; ++i) {
      if (*(data + i) == object) {
        return true;
      }
    }

    return false;
  }

  // Return false if 'ArrayView' does not contain 'other'.
  // The behavior is undefined unless both arrays were initialized.
  bool contains(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size >= other.size) {
      Size i = size - other.size;
      for (Size j = 0; !(i < j); ++j) {
        for (Size k = 0;; ++k) {
          if (k == other.size) {
            return true;
          }

          if (!(*(data + j + k) == *(other.data + k))) {
            break;
          }
        }
      }
    }

    return false;
  }

  // Return false if 'ArrayView' does not contain 'object' at the end.
  // The behavior is undefined unless 'ArrayView' was initialized.
  bool endsWith(const Object& object) const {
    assert(data != nullptr || size == 0);
    if (size == 0) {
      return false;
    }

    return *(data + (size - 1)) == object;
  }

  // Return false if 'ArrayView' does not contain 'other' at the end.
  // The behavior is undefined unless both arrays were initialized.
  bool endsWith(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size >= other.size) {
      Size i = size - other.size;
      for (Size j = 0;; ++j) {
        if (j == other.size) {
          return true;
        }

        if (!(*(data + i + j) == *(other.data + j))) {
          break;
        }
      }
    }

    return false;
  }

  // Looks for the first occurence of 'object' in 'ArrayView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless 'ArrayView' was initialized.
  Size find(const Object& object) const {
    assert(data != nullptr || size == 0);
    for (Size i = 0; i < size; ++i) {
      if (*(data + i) == object) {
        return i;
      }
    }

    return INVALID;
  }

  // Looks for the first occurence of 'other' in 'ArrayView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless both arrays were initialized.
  Size find(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size >= other.size) {
      Size i = size - other.size;
      for (Size j = 0; !(i < j); ++j) {
        for (Size k = 0;; ++k) {
          if (k == other.size) {
            return j;
          }

          if (!(*(data + j + k) == *(other.data + k))) {
            break;
          }
        }
      }
    }

    return INVALID;
  }

  // Looks for the last occurence of 'object' in 'ArrayView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless 'ArrayView' was initialized.
  Size findLast(const Object& object) const {
    assert(data != nullptr || size == 0);
    for (Size i = 0; i < size; ++i) {
      if (*(data + (size - 1 - i)) == object) {
        return size - 1 - i;
      }
    }

    return INVALID;
  }

  // Looks for the first occurence of 'other' in 'ArrayView',
  // returns index or INVALID if there are no occurences.
  // The behavior is undefined unless both arrays were initialized.
  Size findLast(ArrayView other) const {
    assert(data != nullptr || size == 0);
    assert(other.data != nullptr || other.size == 0);
    if (size >= other.size) {
      Size i = size - other.size;
      for (Size j = 0; !(i < j); ++j) {
        for (Size k = 0;; ++k) {
          if (k == other.size) {
            return i - j;
          }

          if (!(*(data + (i - j + k)) == *(other.data + k))) {
            break;
          }
        }
      }
    }

    return INVALID;
  }

  // Returns subarray of 'headSize' first elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'headSize' <= 'size'.
  ArrayView head(Size headSize) const {
    assert(data != nullptr || size == 0);
    assert(headSize <= size);
    return ArrayView(data, headSize);
  }

  // Returns subarray of 'tailSize' last elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'tailSize' <= 'size'.
  ArrayView tail(Size tailSize) const {
    assert(data != nullptr || size == 0);
    assert(tailSize <= size);
    return ArrayView(data + (size - tailSize), tailSize);
  }

  // Returns 'ArrayView' without 'headSize' first elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'headSize' <= 'size'.
  ArrayView unhead(Size headSize) const {
    assert(data != nullptr || size == 0);
    assert(headSize <= size);
    return ArrayView(data + headSize, size - headSize);
  }

  // Returns 'ArrayView' without 'tailSize' last elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'tailSize' <= 'size'.
  ArrayView untail(Size tailSize) const {
    assert(data != nullptr || size == 0);
    assert(tailSize <= size);
    return ArrayView(data, size - tailSize);
  }

  // Returns subarray starting at 'startIndex' and contaning 'endIndex' - 'startIndex' elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'startIndex' <= 'endIndex' and 'endIndex' <= 'size'.
  ArrayView range(Size startIndex, Size endIndex) const {
    assert(data != nullptr || size == 0);
    assert(startIndex <= endIndex && endIndex <= size);
    return ArrayView(data + startIndex, endIndex - startIndex);
  }

  // Returns subarray starting at 'startIndex' and contaning 'sliceSize' elements.
  // The behavior is undefined unless 'ArrayView' was initialized and 'startIndex' <= 'size' and 'startIndex' + 'sliceSize' <= 'size'.
  ArrayView slice(Size startIndex, Size sliceSize) const {
    assert(data != nullptr || size == 0);
    assert(startIndex <= size && startIndex + sliceSize <= size);
    return ArrayView(data + startIndex, sliceSize);
  }

protected:
  const Object* data;
  Size size;
};

template<class Object, class Size> const Size ArrayView<Object, Size>::INVALID = std::numeric_limits<Size>::max();
template<class Object, class Size> const ArrayView<Object, Size> ArrayView<Object, Size>::EMPTY(reinterpret_cast<Object*>(1), 0);
template<class Object, class Size> const ArrayView<Object, Size> ArrayView<Object, Size>::NIL(nullptr, 0);

}
