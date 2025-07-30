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
