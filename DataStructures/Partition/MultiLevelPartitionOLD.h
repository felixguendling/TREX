#pragma once

#include <math.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "../../Helpers/IO/Serialization.h"
#include "../../Helpers/String/String.h"
#include "MultiLevelCell.h"

class MultiLevelPartition {
 public:
  MultiLevelPartition(int NUMBER_OF_ELEMENTS = 0, int NUM_LEVELS = 1,
                      int NUM_CELLS_PER_LEVEL = 1)
      : numLevels(NUM_LEVELS),
        numCellsPerLevel(NUM_CELLS_PER_LEVEL),
        cellIds(NUMBER_OF_ELEMENTS, MultiLevelCell(NUM_LEVELS)){

        };

  inline MultiLevelCell& operator[](int i) {
    AssertMsg((size_t)i < cellIds.size(), "I is out of bounds!");
    return cellIds[i];
  }

  // reads a file (from 'fileName') that has a list of ids which are from [0,
  // numLevels * numCellsPerLevel)
  inline void readFile(std::string fileName) {
    std::fstream file(fileName);

    if (file.is_open()) {
      int globalId(0);
      size_t index(0);

      while (file >> globalId) {
        cellIds[index].setIds(convertGlobalIdToMultiLevelCellId(globalId));
        ++index;
      }

      file.close();

      std::cout << "Read " << String::prettyInt(index) << " many IDs!"
                << std::endl;
    } else {
      std::cerr << "Unable to open the file: " << fileName << std::endl;
    }
  }

  // converts an id from [0, numCellsPerLevel ** numLevels) into a vector of
  // type int (= int) with the correct id per level
  inline std::vector<int> convertGlobalIdToMultiLevelCellId(int globalId) {
    AssertMsg(isGlobalIdValid(globalId),
              "Global ID " << globalId << " is not valid!");

    std::vector<int> ids(numLevels, 0);

    for (int level(0); level < numLevels; ++level) {
      ids[level] = globalId % numCellsPerLevel;
      globalId /= numCellsPerLevel;
    }

    return ids;
  }

  // converts the level ids to the global id
  inline int getGlobalId(MultiLevelCell& cell) {
    int result(0);
    int totalCells = std::pow(numCellsPerLevel, numLevels);

    for (int level(numLevels - 1); level >= 0; --level) {
      totalCells /= numCellsPerLevel;
      result += cell[level] * totalCells;
    }
    return result;
  }

  inline int getGlobalId(int id) { return getGlobalId(cellIds[id]); }

  inline void set(int index, int globalId) {
    AssertMsg(index < (int)cellIds.size(), "Index out of range!");

    cellIds[index].setIds(convertGlobalIdToMultiLevelCellId(globalId));
  }

  // checks if a and b are in the *exact* same cell, top to bottom
  inline bool inSameCell(int a, int b) {
    AssertMsg(a < (int)cellIds.size(), "Element is not valid!");
    AssertMsg(b < (int)cellIds.size(), "Element is not valid!");
    if (a == b) return true;
    return inSameCell(cellIds[a], cellIds[b]);
  }

  inline bool inSameCell(int a, std::vector<int>& levels,
                         std::vector<int>& cellIds) {
    AssertMsg(levels.size() == cellIds.size(),
              "Levels and CellIds must have same size!");

    bool result = true;
    for (size_t i(0); i < levels.size(); ++i) {
      result &= (getCell(levels[i], a) == cellIds[i]);
    }
    return result;
  }

  inline bool inSameCell(MultiLevelCell& a, MultiLevelCell& b) {
    bool result = true;
    for (int level(0); level < numLevels; ++level) {
      result &= (a[level] == b[level]);
    }
    return result;
  }

  inline int getLowestCommonLevel(int a, int b) {
    AssertMsg(a < (int)cellIds.size(), "Element is not valid!");
    AssertMsg(b < (int)cellIds.size(), "Element is not valid!");

    if (a == b) return 0;

    for (int level(numLevels - 1); level >= 0; --level) {
      if (cellIds[a][level] != cellIds[b][level]) return level + 1;
    }
    return 0;
  }

  // return the level where a and b cross (-1 if they don't cross)
  inline int findCrossingLevel(int a, int b) {
    AssertMsg(a < (int)cellIds.size(), "Element is not valid!");
    AssertMsg(b < (int)cellIds.size(), "Element is not valid!");
    if (a == b) return numLevels;

    for (int level(0); level < numLevels; ++level) {
      if (cellIds[a][level] != cellIds[b][level]) return level;
    }
    return numLevels;
  }

  // ** Getter **
  // returns all id's of elements with [level] == localId
  inline std::vector<int> verticesInCell(int level, int localId) {
    AssertMsg(level < numLevels, "Level is not valid!");
    AssertMsg(localId < numCellsPerLevel, "localId is not valid!");

    std::vector<int> elements;

    for (int i(0); i < (int)cellIds.size(); ++i) {
      if (cellIds[i][level] == localId) elements.push_back(i);
    }

    return elements;
  }

  inline std::vector<int> getCellIds(int a) noexcept {
    AssertMsg(a < (int)cellIds.size(), "Element is not valid!");

    return cellIds[a].getIds();
  }

  inline int getCell(int level, int a) {
    AssertMsg(a < (int)cellIds.size(), "Element is not valid!");
    AssertMsg(level < (int)numLevels, "Level is not valid!");
    return cellIds[a][level];
  }

  inline std::vector<int> get(MultiLevelCell& cell) {
    std::vector<int> elements;
    elements.reserve(cellIds.size());

    for (int i(0); i < (int)cellIds.size(); ++i) {
      if (inSameCell(cell, cellIds[i])) elements.push_back(i);
    }

    return elements;
  }

  // returns all id's of elements that share the same id's on the given levels
  inline std::vector<int> verticesInCell(std::vector<int> levels,
                                         std::vector<int> localIds) {
    AssertMsg(levels.size() == localIds.size(),
              "Two vectors have different length!");
    AssertMsg(std::all_of(levels.begin(), levels.end(),
                          [&](int level) { return level < numLevels; }),
              "Level is not valid!");
    AssertMsg(
        std::all_of(localIds.begin(), localIds.end(),
                    [&](int localId) { return localId < numCellsPerLevel; }),
        "localId is not valid!");

    std::vector<int> elements;

    for (int i(0); i < (int)cellIds.size(); ++i) {
      bool valid = true;
      for (int j(0); j < (int)levels.size(); ++j) {
        valid &= (cellIds[i][levels[j]] == localIds[j]);
      }
      if (valid) elements.push_back(i);
    }

    return elements;
  }

  inline size_t getNumberOfLevels() const { return (size_t)numLevels; }

  inline size_t getNumberOfCellsInLevel(int level) const {
    AssertMsg(isLevelValid(level), "Level is not valid!");
    return getNumberOfLevels();
  }

  inline int getNumberOfCellsPerLevel() const { return numCellsPerLevel; }

  inline size_t getNumElements() const { return cellIds.size(); }

  inline std::vector<MultiLevelCell>& getIds() { return cellIds; }

  // globalId in range [0, std::pow(numCellsPerLevel, numLevels))
  inline bool isGlobalIdValid(int globalId) {
    return (0 <= globalId && globalId < std::pow(numCellsPerLevel, numLevels));
  }

  // check if local cell id is valid
  inline bool isLocalIdValid(int localId) const {
    return (0 <= localId && localId < numCellsPerLevel);
  }

  inline bool isLevelValid(int level) const {
    return 0 <= level && level < numLevels;
  }

  inline void serialize(IO::Serialization& serialize) const noexcept {
    serialize(numLevels, numCellsPerLevel, cellIds);
  }

  inline void deserialize(IO::Deserialization& deserialize) noexcept {
    deserialize(numLevels, numCellsPerLevel, cellIds);
  }

 private:
  int numLevels;
  int numCellsPerLevel;
  std::vector<MultiLevelCell> cellIds;
};
