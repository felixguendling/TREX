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

#include <vector>

#include "../../Helpers/Assert.h"
#include "../../Helpers/IO/Serialization.h"

class MultiLevelCell {
 public:
  MultiLevelCell(int NUM_LEVELS = 1) : ids(NUM_LEVELS, 0){};

  // returns true if the 'level' is a valid level, not negative or too large.
  inline bool isValidLevel(int level) {
    return (0 <= level && (size_t)level < ids.size());
  }

  // operator [i] returns the id of the i'th level
  inline int& operator[](int level) {
    AssertMsg(isValidLevel(level), "Level is out of bounds!");
    return ids[level];
  }

  inline std::vector<int> getIds() { return ids; }

  // sets the correct ids
  inline void setIds(std::vector<int> newIds) { ids = newIds; }

  inline int getLevels() const { return (int)ids.size(); }

  friend std::ostream& operator<<(std::ostream& out, MultiLevelCell& s) {
    for (int level(0); level < s.getLevels(); ++level) {
      out << (int)s[level] << " ";
    }
    return out;
  }

  inline void serialize(IO::Serialization& serialize) const noexcept {
    serialize(ids);
  }

  inline void deserialize(IO::Deserialization& deserialize) noexcept {
    deserialize(ids);
  }

 private:
  std::vector<int> ids;
};
