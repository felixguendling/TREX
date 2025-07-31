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

#include "Data.h"
#include "TransferModes.h"

namespace RAPTOR {

using TransferGraph = ::TransferGraph;

class MultimodalData {
 public:
  MultimodalData(const std::string& fileName) { deserialize(fileName); }

  MultimodalData(const Data& data) : raptorData(data) {}

 public:
  inline void serialize(const std::string& fileName) const noexcept {
    IO::serialize(fileName, modes);
    raptorData.serialize(fileName + ".raptor");
    for (const size_t mode : modes) {
      transferGraphs[mode].writeBinary(fileName + "." +
                                       TransferModeNames[mode] + ".graph");
    }
  }

  inline void deserialize(const std::string& fileName) noexcept {
    IO::deserialize(fileName, modes);
    raptorData.deserialize(fileName + ".raptor");
    for (const size_t mode : modes) {
      transferGraphs[mode].readBinary(fileName + "." + TransferModeNames[mode] +
                                      ".graph");
    }
  }

  inline void useImplicitDepartureBufferTimes() noexcept {
    raptorData.useImplicitDepartureBufferTimes();
  }

  inline void dontUseImplicitDepartureBufferTimes() noexcept {
    raptorData.dontUseImplicitDepartureBufferTimes();
  }

  inline void useImplicitArrivalBufferTimes() noexcept {
    raptorData.useImplicitArrivalBufferTimes();
  }

  inline void dontUseImplicitArrivalBufferTimes() noexcept {
    raptorData.dontUseImplicitArrivalBufferTimes();
  }

  inline void printInfo() const noexcept {
    std::cout << "RAPTOR data:" << std::endl;
    raptorData.printInfo();
    for (const size_t mode : modes) {
      std::cout << "Graph for " << TransferModeNames[mode] << ":" << std::endl;
      Graph::printInfo(transferGraphs[mode]);
    }
  }

  inline void addTransferGraph(const size_t mode,
                               const TransferGraph& graph) noexcept {
    AssertMsg(mode < NUM_TRANSFER_MODES, "Mode is not supported!");
    if (!Vector::contains(modes, mode)) {
      modes.emplace_back(mode);
    }
    transferGraphs[mode] = graph;
  }

  inline const TransferGraph& getTransferGraph(
      const size_t mode) const noexcept {
    AssertMsg(Vector::contains(modes, mode), "Mode is not supported!");
    return transferGraphs[mode];
  }

  inline Data getBimodalData(const size_t mode) const noexcept {
    Data resultData(raptorData);
    resultData.transferGraph = getTransferGraph(mode);
    return resultData;
  }

  inline Data getPruningData() const noexcept { return getPruningData(modes); }

  inline Data getPruningData(
      const std::vector<size_t>& pruningModes) const noexcept {
    AssertMsg(!pruningModes.empty(),
              "Cannot build pruning data without transfer modes!");
    Data resultData(raptorData);
    DynamicTransferGraph temp;
    Graph::copy(raptorData.transferGraph, temp);
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
    Graph::move(std::move(temp), resultData.transferGraph);
    return resultData;
  }

 public:
  Data raptorData;
  std::vector<size_t> modes;
  TransferGraph transferGraphs[NUM_TRANSFER_MODES];
};

}  // namespace RAPTOR
