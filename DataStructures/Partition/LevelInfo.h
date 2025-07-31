/**********************************************************************************

 Copyright (c) 2023-2025 Patrick Steil
 Copyright (c) 2019-2022 KIT ITI Algorithmics Group

 MIT License

 Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************************/
#pragma once

// taken and adapted from here
// https://github.com/michaelwegner/CRP/blob/master/datastructures/LevelInfo.h

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../../Helpers/Assert.h"
#include "../../Helpers/IO/Serialization.h"

class LevelInfo {
 private:
  std::vector<uint8_t> offset;

 public:
  LevelInfo() = default;
  LevelInfo(const std::vector<uint8_t>& offset) : offset(offset) {}

  size_t getCellNumberOnLevel(size_t l, size_t cellNumber) const {
    AssertMsg(0 < l && l < offset.size(), "Level out of bounds!");
    return (cellNumber & ~(~0U << offset[l])) >> offset[l - 1];
  }

  // returns numberOfLevels if in same cell
  size_t getHighestDifferingLevel(const size_t c1, const size_t c2) const {
    size_t diff = c1 ^ c2;
    if (diff == 0) return offset.size();

    for (int l = (int)offset.size() - 1; l > 0; --l) {
      if (diff >> offset[l - 1] > 0) return l;
    }
    return offset.size();
  }

  size_t truncateToLevel(size_t cellNumber, size_t l) const {
    AssertMsg(0 < l && l <= getLevelCount(), "Level out of bounds");
    return cellNumber >> offset[l - 1];
  }

  size_t getLevelCount() const { return offset.size() - 1; }

  const std::vector<uint8_t>& getOffsets() const { return offset; }

  void serialize(IO::Serialization& serialize) const noexcept {
    serialize(offset);
  }

  void deserialize(IO::Deserialization& deserialize) noexcept {
    deserialize(offset);
  }
};
