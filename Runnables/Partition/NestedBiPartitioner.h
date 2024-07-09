#pragma once

#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>

#include "../../ExternalLibs/KaHIP/interface/kaHIP_interface.h"

class Partitioner {
public:
  Partitioner()
      : toAdj(), toVertex(), toWeight(), vertexWeight(), cellIds(), mapping(),
        numLevels(0){};

  Partitioner(std::vector<size_t> toAdj, std::vector<size_t> toVertex,
              std::vector<size_t> toWeight, std::vector<size_t> vertexWeight,
              size_t numLevels)
      : toAdj(toAdj), toVertex(toVertex), toWeight(toWeight),
        vertexWeight(vertexWeight), cellIds(vertexWeight.size(), 0),
        mapping(vertexWeight.size(), 0), numLevels(numLevels){};

  void setNumLevels(size_t newNumLevels) { numLevels = newNumLevels; }

  void clear() { cellIds.assign(vertexWeight.size(), 0); }

  void reset() {
    toAdj.clear();
    toVertex.clear();
    toWeight.clear();
    vertexWeight.clear();
    cellIds.clear();
    mapping.clear();
  }

  void writePartitionToFile(const std::string &fileName) {
    std::cout << "Writing all the cell ids to the file '" << fileName << "'\n";
    std::ofstream file(fileName);

    for (auto &cellId : cellIds) {
      file << cellId << "\n";
    }
    file.close();
  }

  void startNestedBipartition() {
    clear();

    for (size_t currentLevel(numLevels); currentLevel > 0; --currentLevel) {
      runBipartitionForLevel(currentLevel - 1);
    }
  }

private:
  template <bool VERBOSE = true>
  void runBipartitionForLevel(size_t currentLevel) {
    // currentLevel = numLevels -1 <=> top level
    // currentLevel = 0 <=> bottom level

    // helper function, returns true iff on this currentLevel, the given cellId
    // is set
    auto isValid = [&](size_t currentCellId, size_t cellId) {
      return (currentCellId >> (currentLevel + 1) == cellId);
    };

    for (int i(0); i < (1 << (numLevels - currentLevel - 1)); ++i) {
      if (VERBOSE)
        std::cout << "[Level] " << (currentLevel + 1) << " [cellId] "
                  << std::bitset<32>(i) << std::endl;

      auto verticesOfCell = extractVertices(i, isValid);
      size_t numVerticesSubgraph = verticesOfCell.size();

      // create subgraph datastructures
      std::vector<size_t> subgraphToAdjVec(numVerticesSubgraph + 1, 0);
      std::vector<size_t> subgraphToVertexVec;
      std::vector<size_t> subgraphToWeightVec;
      std::vector<size_t> subgraphVertexWeightVec(numVerticesSubgraph, 0);

      // create mapping
      for (size_t j(0); j < verticesOfCell.size(); ++j) {
        mapping[verticesOfCell[j]] = j;
      }

      // fill datastructures
      size_t runningSum(0);

      for (size_t j(0); j < verticesOfCell.size(); ++j) {
        auto &node = verticesOfCell[j];
        subgraphVertexWeightVec[j] = vertexWeight[node];
        subgraphToAdjVec[j] = runningSum;

        for (size_t toIndex(toAdj[node]); toIndex < toAdj[node + 1];
             ++toIndex) {
          // check if toVertex is in same subgraph
          if (!isValid(cellIds[toVertex[toIndex]], i))
            continue;

          subgraphToVertexVec.push_back(mapping[toVertex[toIndex]]);
          subgraphToWeightVec.push_back(toWeight[toIndex]);

          ++runningSum;
        }
      }

      subgraphToAdjVec.back() = runningSum;

      if (VERBOSE) {
        size_t totalWeight(0);
        for (size_t i(0); i < numVerticesSubgraph; ++i)
          totalWeight += subgraphVertexWeightVec[i];
        std::cout << "# of vertices:  " << numVerticesSubgraph
                  << ", weight: " << totalWeight << std::endl;
      }

      // convert to array
      // Convert vectors to int arrays
      int *subgraphToAdj = new int[numVerticesSubgraph + 1];
      int *subgraphToVertex = new int[subgraphToVertexVec.size()];
      int *subgraphToWeight = new int[subgraphToWeightVec.size()];
      int *subgraphVertexWeight = new int[numVerticesSubgraph];

      std::copy(subgraphToAdjVec.begin(), subgraphToAdjVec.end(),
                subgraphToAdj);
      std::copy(subgraphToVertexVec.begin(), subgraphToVertexVec.end(),
                subgraphToVertex);
      std::copy(subgraphToWeightVec.begin(), subgraphToWeightVec.end(),
                subgraphToWeight);
      std::copy(subgraphVertexWeightVec.begin(), subgraphVertexWeightVec.end(),
                subgraphVertexWeight);

      int n = numVerticesSubgraph;
      double imbalance = 0.03;
      int *part = new int[n];
      int edge_cut = 0;
      int nparts = 2;

      if (VERBOSE)
        std::cout << "Start kaHIP..." << std::endl;

      // call partition
      kaffpa(&n, subgraphVertexWeight, subgraphToAdj, subgraphToWeight,
             subgraphToVertex, &nparts, &imbalance, false, 0, STRONGSOCIAL,
             &edge_cut, part);

      if (VERBOSE)
        std::cout << "done. [edge cut: " << edge_cut << "]" << std::endl;

      // rewrite cell ids for this level
      for (size_t j(0); j < verticesOfCell.size(); ++j) {
        auto &node = verticesOfCell[j];

        cellIds[node] |= (part[j] << (currentLevel));
      }

      // Don't forget to free the allocated memory
      delete[] subgraphToAdj;
      delete[] subgraphToVertex;
      delete[] subgraphToWeight;
      delete[] subgraphVertexWeight;

      for (size_t j(0); j < verticesOfCell.size(); ++j) {
        mapping[verticesOfCell[j]] = 0;
      }
    }
  }

  template <typename FUNCTION>
  std::vector<size_t> extractVertices(size_t cellId, const FUNCTION &isValid) {
    std::vector<size_t> result;
    for (size_t index(0); index < cellIds.size(); ++index) {
      if (isValid(cellIds[index], cellId)) {
        result.push_back(index);
      }
    }
    return result;
  }

private:
  // datastructures for the graph
  std::vector<size_t> toAdj;
  std::vector<size_t> toVertex;
  std::vector<size_t> toWeight;
  std::vector<size_t> vertexWeight;

  // stores the final cell ids
  std::vector<size_t> cellIds;

  // mapping
  std::vector<size_t> mapping;

  // number of levels
  size_t numLevels;
};
