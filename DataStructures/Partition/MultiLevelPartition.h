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

// Code taken and adapted from here
// (https://github.com/michaelwegner/CRP/blob/master/datastructures/MultiLevelPartition.h)

#include <stddef.h>

#include <cmath>
#include <vector>

#include "../../Helpers/Assert.h"
#include "../../Helpers/IO/Serialization.h"

class MultiLevelPartition {
 public:
  MultiLevelPartition(size_t numVertices = 0, size_t numLevels = 0,
                      size_t numCellsInLevel = 0) {
    setNumberOfLevels(numLevels);
    setNumberOfCellsPerLevel(numCellsInLevel);
    setNumberOfVertices(numVertices);
    computeBitmap();
  }

  void setNumberOfLevels(size_t numLevels) { numCells.assign(numLevels, 0); }

  void setNumberOfVertices(size_t numVertices) {
    cellNumbers.assign(numVertices, 0);
  }

  // this is to specifically set the number of cells at a certain level ...
  void setNumberOfCellsInLevel(size_t level, size_t numberOfCells) {
    AssertMsg(isLevelValid(level), "Level is not valid!");
    numCells[level] = numberOfCells;
  }

  // .. and this is to set the given numberOfCells to all levels
  void setNumberOfCellsPerLevel(size_t numberOfCells) {
    for (size_t level(0); level < getNumberOfLevels(); ++level) {
      setNumberOfCellsInLevel(level, numberOfCells);
    }
  }

  // important to compute all the correct offsets and stuff
  void computeBitmap() {
    pvOffset = std::vector<uint8_t>(numCells.size() + 1);
    for (size_t i = 0; i < numCells.size(); ++i) {
      pvOffset[i + 1] = pvOffset[i] + ceil(log2(numCells[i]));
    }
  }

  void setCell(uint8_t level, size_t vertexId, size_t cellId) {
    AssertMsg(isLevelValid(level), "Level is not valid!");
    AssertMsg(isVertexValid(vertexId), "Vertex is not valid!");
    AssertMsg(isCellIdValid(cellId, level), "CellId is not valid!");

    cellNumbers[vertexId] |= (((size_t)cellId) << pvOffset[level]);
  }

  void setCell(size_t vertexId, size_t globalId) {
    AssertMsg(globalId < getTotalNumberOfCells(), "GlobalID is out of bounds!");
    for (size_t level(0); level < getNumberOfLevels(); ++level) {
      setCell(level, vertexId, globalId % getNumberOfCellsInLevel(level));
      globalId /= getNumberOfCellsInLevel(level);
    }
  }

  // next methods allow to compare vertices

  // returns the highest crossing level, i.e., the highest level l where a and b
  // are no longer in the same cellid on level l
  size_t findCrossingLevel(size_t a, size_t b) const {
    AssertMsg(isVertexValid(a) && isVertexValid(b), "Invalid vertex IDs!");

    size_t numLevels = getNumberOfLevels();
    for (size_t level(0); level < numLevels; ++level) {
      size_t cell1 = getCell(numLevels - level - 1, a);
      size_t cell2 = getCell(numLevels - level - 1, b);
      if (cell1 != cell2) {
        return numLevels - level - 1;
      }
    }

    return numLevels;
  }

  // returns true if a and b are in the exact same level
  bool inSameCell(size_t a, size_t b) const {
    AssertMsg(isVertexValid(a) && isVertexValid(b), "Invalid vertex IDs!");
    return (cellNumbers[a] == cellNumbers[b]);
  }

  bool inSameCell(size_t a, std::vector<int> levels,
                  std::vector<int> cellIds) const {
    AssertMsg(isVertexValid(a), "Vertex is not valid!");

    bool result = true;
    for (size_t i(0); i < levels.size(); ++i) {
      AssertMsg(isLevelValid(levels[i]), "Level is not valid!");
      AssertMsg(isCellIdValid(cellIds[i], levels[i]), "CellId is not valid!");
      result &= (getCell((uint8_t)levels[i], a) == (size_t)cellIds[i]);
    }
    return result;
  }

  // Return the cellId of vertex in the given level
  size_t getCell(uint8_t level, size_t vertexId) const {
    AssertMsg(isLevelValid(level), "Level " << (int)level << " is not valid!");
    AssertMsg(isVertexValid(vertexId), "Vertex is not valid!");

    return (cellNumbers[vertexId] >> pvOffset[level]) &
           ~(~0U << pvOffset[level + 1]);
  }

  std::vector<size_t> verticesInCell(std::vector<int> levels,
                                     std::vector<int> cellIds) {
    AssertMsg(levels.size() == cellIds.size(),
              "Levels and CellIDs need to have the same length!");

    std::vector<size_t> result;
    result.reserve(getNumberOfVertices());

    for (size_t vertex(0); vertex < getNumberOfVertices(); ++vertex) {
      bool insert = true;

      for (size_t i(0); i < levels.size(); ++i) {
        insert &= (getCell(levels[i], vertex) == (size_t)cellIds[i]);
      }

      if (insert) result.push_back(vertex);
    }

    return result;
  }

  size_t getNumberOfVertices() const { return cellNumbers.size(); }

  size_t getNumberOfLevels() const { return numCells.size(); }

  size_t getNumberOfCellsPerLevel() const {
    // this assumes that all levels have the same number of cells
    return numCells[0];
  }

  size_t getNumberOfCellsInLevel(uint8_t l) const {
    AssertMsg(isLevelValid(l), "Level is not valid!");
    return numCells[l];
  }

  size_t getTotalNumberOfCells() const {
    size_t total(1);

    for (size_t level(0); level < getNumberOfLevels(); ++level) {
      total *= getNumberOfCellsInLevel(level);
    }

    return total;
  }

  std::vector<uint8_t> getPVOffsets() { return pvOffset; }

  // returns the cell numbers of
  size_t getCellNumber(size_t vertexId) const {
    AssertMsg(isVertexValid(vertexId), "Vertex is not valid");
    return cellNumbers[vertexId];
  }

  // Helper for Assertions
  bool isLevelValid(uint8_t level) const { return level < numCells.size(); }

  bool isVertexValid(size_t u) const { return u < cellNumbers.size(); }

  bool isCellIdValid(size_t cellId, uint8_t level) const {
    return cellId < numCells[level];
  }

  void write(const std::string fileName) const noexcept {
    std::ofstream file;
    file.open(fileName);
    if (file.is_open()) {
      file << numCells.size() << std::endl;
      for (size_t i = 0; i < numCells.size(); ++i) {
        file << numCells[i] << std::endl;
      }
      file << getNumberOfVertices() << std::endl;
      for (size_t i = 0; i < getNumberOfVertices(); ++i) {
        file << cellNumbers[i] << std::endl;
      }

      file.close();
    }
  }

  void read(const std::string fileName) noexcept {
    std::ifstream file;
    file.open(fileName);
    if (file.is_open()) {
      std::string line;
      getline(file, line);

      size_t numLevels = std::stoi(line);
      numCells = std::vector<size_t>(numLevels);
      for (size_t i = 0; i < numLevels; ++i) {
        std::getline(file, line);
        numCells[i] = std::stoi(line);
      }

      computeBitmap();

      std::getline(file, line);
      size_t numVertices = std::stoi(line);

      cellNumbers = std::vector<size_t>(numVertices);
      for (size_t i = 0; i < numVertices; ++i) {
        std::getline(file, line);
        cellNumbers[i] = std::stoull(line);
      }
    }
  }

  void serialize(IO::Serialization& serialize) const noexcept {
    serialize(numCells, pvOffset, cellNumbers);
  }

  void deserialize(IO::Deserialization& deserialize) noexcept {
    deserialize(numCells, pvOffset, cellNumbers);
  }

 private:
  std::vector<size_t> numCells;
  std::vector<uint8_t> pvOffset;
  std::vector<size_t> cellNumbers;
};
