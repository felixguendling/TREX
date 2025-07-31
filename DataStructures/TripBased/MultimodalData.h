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

#include "../RAPTOR/TransferModes.h"
#include "Data.h"

namespace TripBased {

using TransferGraph = ::TransferGraph;

class MultimodalData {
 public:
  MultimodalData(const std::string& fileName) { deserialize(fileName); }

  MultimodalData(const Data& data) : tripData(data) {}

 public:
  inline void serialize(const std::string& fileName) const noexcept {
    IO::serialize(fileName, modes);
    tripData.serialize(fileName + ".trip");
    for (const size_t mode : modes) {
      stopEventGraphs[mode].writeBinary(
          fileName + "." + RAPTOR::TransferModeNames[mode] + ".graph");
    }
  }

  inline void deserialize(const std::string& fileName) noexcept {
    IO::deserialize(fileName, modes);
    tripData.deserialize(fileName + ".trip");
    for (const size_t mode : modes) {
      stopEventGraphs[mode].readBinary(
          fileName + "." + RAPTOR::TransferModeNames[mode] + ".graph");
    }
  }

  inline void printInfo() const noexcept {
    std::cout << "Trip-Based data:" << std::endl;
    tripData.printInfo();
    for (const size_t mode : modes) {
      std::cout << "Graph for " << RAPTOR::TransferModeNames[mode] << ":"
                << std::endl;
      Graph::printInfo(stopEventGraphs[mode]);
    }
  }

  inline void addTransferGraph(const size_t mode,
                               const TransferGraph& graph) noexcept {
    AssertMsg(mode < RAPTOR::NUM_TRANSFER_MODES, "Mode is not supported!");
    if (!Vector::contains(modes, mode)) {
      modes.emplace_back(mode);
    }
    stopEventGraphs[mode] = graph;
  }

  inline const TransferGraph& getTransferGraph(
      const size_t mode) const noexcept {
    AssertMsg(Vector::contains(modes, mode), "Mode is not supported!");
    return stopEventGraphs[mode];
  }

  inline Data getBimodalData(const size_t mode) const noexcept {
    Data resultData(tripData);
    // Arc-Flag TB
    // resultData.stopEventGraph = getTransferGraph(mode);
    Graph::copy(getTransferGraph(mode), resultData.stopEventGraph);
    return resultData;
  }

  inline Data getPruningData() const noexcept { return getPruningData(modes); }

  inline Data getPruningData(
      const std::vector<size_t>& pruningModes) const noexcept {
    AssertMsg(!pruningModes.empty(),
              "Cannot build pruning data without transfer modes!");
    Data resultData(tripData);
    DynamicTransferGraph temp;
    Graph::copy(tripData.stopEventGraph, temp);
    for (const size_t mode : pruningModes) {
      const TransferGraph& graph = getTransferGraph(mode);
      for (const Vertex from : graph.vertices()) {
        for (const Edge edge : graph.edgesFrom(from)) {
          const Vertex to = graph.get(ToVertex, edge);
          const Edge originalEdge = temp.findEdge(from, to);
          if (originalEdge == noEdge) {
            temp.addEdge(from, to, graph.edgeRecord(edge));
          } else {
            const int travelTime = graph.get(TravelTime, edge);
            const int originalTravelTime = temp.get(TravelTime, originalEdge);
            temp.set(TravelTime, originalEdge,
                     std::min(travelTime, originalTravelTime));
          }
        }
      }
    }
    Graph::move(std::move(temp), resultData.stopEventGraph);
    return resultData;
  }

 public:
  Data tripData;
  std::vector<size_t> modes;
  TransferGraph stopEventGraphs[RAPTOR::NUM_TRANSFER_MODES];
};

}  // namespace TripBased
