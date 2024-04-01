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


#include "StreamTools.h"
#include <stdexcept>
#include "IInputStream.h"
#include "IOutputStream.h"

namespace Common {

void read(IInputStream& in, void* data, size_t size) {
  while (size > 0) {
    size_t readSize = in.readSome(data, size);
    if (readSize == 0) {
      throw std::runtime_error("Failed to read from IInputStream");
    }

    data = static_cast<uint8_t*>(data) + readSize;
    size -= readSize;
  }
}

void read(IInputStream& in, int8_t& value) {
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, int16_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, int32_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, int64_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, uint8_t& value) {
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, uint16_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, uint32_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, uint64_t& value) {
  // TODO: Convert from little endian on big endian platforms
  read(in, &value, sizeof(value));
}

void read(IInputStream& in, std::vector<uint8_t>& data, size_t size) {
  data.resize(size);
  read(in, data.data(), size);
}

void read(IInputStream& in, std::string& data, size_t size) {
  std::vector<char> temp(size);
  read(in, temp.data(), size);
  data.assign(temp.data(), size);
}

void readVarint(IInputStream& in, uint8_t& value) {
  uint8_t temp = 0;
  for (uint8_t shift = 0;; shift += 7) {
    uint8_t piece;
    read(in, piece);
    if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
      throw std::runtime_error("readVarint, value overflow");
    }

    temp |= static_cast<size_t>(piece & 0x7f) << shift;
    if ((piece & 0x80) == 0) {
      if (piece == 0 && shift != 0) {
        throw std::runtime_error("readVarint, invalid value representation");
      }

      break;
    }
  }

  value = temp;
}

void readVarint(IInputStream& in, uint16_t& value) {
  uint16_t temp = 0;
  for (uint8_t shift = 0;; shift += 7) {
    uint8_t piece;
    read(in, piece);
    if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
      throw std::runtime_error("readVarint, value overflow");
    }

    temp |= static_cast<size_t>(piece & 0x7f) << shift;
    if ((piece & 0x80) == 0) {
      if (piece == 0 && shift != 0) {
        throw std::runtime_error("readVarint, invalid value representation");
      }

      break;
    }
  }

  value = temp;
}

void readVarint(IInputStream& in, uint32_t& value) {
  uint32_t temp = 0;
  for (uint8_t shift = 0;; shift += 7) {
    uint8_t piece;
    read(in, piece);
    if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
      throw std::runtime_error("readVarint, value overflow");
    }

    temp |= static_cast<size_t>(piece & 0x7f) << shift;
    if ((piece & 0x80) == 0) {
      if (piece == 0 && shift != 0) {
        throw std::runtime_error("readVarint, invalid value representation");
      }

      break;
    }
  }

  value = temp;
}

void readVarint(IInputStream& in, uint64_t& value) {
  uint64_t temp = 0;
  for (uint8_t shift = 0;; shift += 7) {
    uint8_t piece;
    read(in, piece);
    if (shift >= sizeof(temp) * 8 - 7 && piece >= 1 << (sizeof(temp) * 8 - shift)) {
      throw std::runtime_error("readVarint, value overflow");
    }

    temp |= static_cast<uint64_t>(piece & 0x7f) << shift;
    if ((piece & 0x80) == 0) {
      if (piece == 0 && shift != 0) {
        throw std::runtime_error("readVarint, invalid value representation");
      }

      break;
    }
  }

  value = temp;
}

void write(IOutputStream& out, const void* data, size_t size) {
  while (size > 0) {
    size_t writtenSize = out.writeSome(data, size);
    if (writtenSize == 0) {
      throw std::runtime_error("Failed to write to IOutputStream");
    }

    data = static_cast<const uint8_t*>(data) + writtenSize;
    size -= writtenSize;
  }
}

void write(IOutputStream& out, int8_t value) {
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, int16_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, int32_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, int64_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, uint8_t value) {
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, uint16_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, uint32_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, uint64_t value) {
  // TODO: Convert to little endian on big endian platforms
  write(out, &value, sizeof(value));
}

void write(IOutputStream& out, const std::vector<uint8_t>& data) {
  write(out, data.data(), data.size());
}

void write(IOutputStream& out, const std::string& data) {
  write(out, data.data(), data.size());
}

void writeVarint(IOutputStream& out, uint32_t value) {
  while (value >= 0x80) {
    write(out, static_cast<uint8_t>(value | 0x80));
    value >>= 7;
  }

  write(out, static_cast<uint8_t>(value));
}

void writeVarint(IOutputStream& out, uint64_t value) {
  while (value >= 0x80) {
    write(out, static_cast<uint8_t>(value | 0x80));
    value >>= 7;
  }

  write(out, static_cast<uint8_t>(value));
}

}
