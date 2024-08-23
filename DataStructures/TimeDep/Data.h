#pragma once

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

#include "../../Helpers/Assert.h"
#include "../../Helpers/IO/Serialization.h"
#include "../../Helpers/Ranges/Range.h"
#include "../../Helpers/Ranges/SubRange.h"
#include "../Graph/Graph.h"
#include "../Intermediate/Data.h"

namespace TDD {

class Data {
private:
    struct ConnectionToBuild {
        StopId fromStop;
        StopId toStop;
        int departureTime;
        int arrivalTime;

        ConnectionToBuild(StopId fromStop, StopId toStop, int departureTime, int arrivalTime)
            : fromStop(fromStop)
            , toStop(toStop)
            , departureTime(departureTime)
            , arrivalTime(arrivalTime)
        {
        }

        auto operator<=>(const ConnectionToBuild& other)
        {
            return std::tie(fromStop, toStop, departureTime, arrivalTime) <=> std::tie(other.fromStop, other.toStop, other.departureTime, other.arrivalTime);
        }
    };

public:
    Data() { }

    Data(const std::string& fileName)
    {
        deserialize(fileName);
    }

    const auto& getGraph() const { return graph; }
    const auto& getEdgeWeights() const { return edgeWeights; }

    inline static Data FromBinary(const std::string& fileName) noexcept
    {
        Data data;
        data.deserialize(fileName);
        return data;
    }

    inline static Data FromIntermediate(const Intermediate::Data& inter) noexcept
    {
        Data data;

        std::vector<ConnectionToBuild> connections;

        for (const Intermediate::Trip& trip : inter.trips) {
            AssertMsg(!trip.stopEvents.empty(), "Intermediate data contains trip without any stop event!");
            for (size_t i = 1; i < trip.stopEvents.size(); i++) {
                const Intermediate::StopEvent& from = trip.stopEvents[i - 1];
                const Intermediate::StopEvent& to = trip.stopEvents[i];
                connections.emplace_back(from.stopId, to.stopId, from.departureTime, to.arrivalTime);
            }
        }

        std::sort(connections.begin(), connections.end());

        size_t resultingNumberOfEdges = 1;

        for (size_t i = 1; i < connections.size(); ++i) {
            resultingNumberOfEdges += (connections[i - 1].fromStop != connections[i].fromStop || connections[i - 1].toStop != connections[i].toStop);
        }

        data.edgeWeights.resize(resultingNumberOfEdges);

        DynamicTDDGraph builderGraph;
        builderGraph.addVertices(inter.stops.size());

        // Add the connections
        size_t currentIndex = 0;
        data.edgeWeights[currentIndex].emplace_back(connections[0].departureTime, connections[0].arrivalTime - connections[0].departureTime);

        for (size_t i = 1; i < connections.size(); ++i) {
            if (connections[i - 1].fromStop != connections[i].fromStop || connections[i - 1].toStop != connections[i].toStop) {
                builderGraph.addEdge(connections[i - 1].fromStop, connections[i - 1].toStop).set(Index, data.edgeWeights.size());

                ++currentIndex;
            }
            data.edgeWeights[currentIndex].emplace_back(connections[i].departureTime, connections[i].arrivalTime - connections[i].departureTime);
        }
        assert(currentIndex < data.edgeWeights.size());
        builderGraph.addEdge(connections.back().fromStop, connections.back().toStop).set(Index, currentIndex);

        for (Edge edge(0); edge < builderGraph.numEdges(); ++edge) {
            builderGraph.set(TravelTime, edge, -1);
        }

        // Add the footpath edges
        for (const auto [transferEdge, from] : inter.transferGraph.edgesWithFromVertex()) {
            Vertex to = inter.transferGraph.get(ToVertex, transferEdge);
            int duration = inter.transferGraph.get(TravelTime, transferEdge);
            if (to == Vertex(from))
                continue;

            builderGraph.addEdge(from, to).set(TravelTime, duration);
        }

        Graph::move(std::move(builderGraph), data.graph);

        // add sentinels
        for (auto& weights : data.edgeWeights) {
            weights.emplace_back(INFTY, INFTY);
        }

        return data;
    }

public:
    inline size_t numberOfStops() const noexcept { return graph.numVertices(); }
    inline size_t numberOfEdges() const noexcept { return graph.numEdges(); }

    inline bool isStop(const Vertex stop) const noexcept
    {
        return stop < numberOfStops();
    }
    inline Range<StopId> stops() const noexcept
    {
        return Range<StopId>(StopId(0), StopId(numberOfStops()));
    }

    inline void printInfo() const noexcept
    {
        std::cout << "TDD public transit data:" << std::endl;
        std::cout << "   Number of Stops:          " << std::setw(12) << String::prettyInt(numberOfStops()) << std::endl;
        std::cout << "   Number of Edges:          " << std::setw(12) << String::prettyInt(numberOfEdges()) << std::endl;
        std::cout << "   Size:                     " << std::setw(12) << String::bytesToString(byteSize()) << std::endl;
    }

    inline void serialize(const std::string& fileName) const noexcept
    {
        IO::serialize(fileName, edgeWeights);
        graph.writeBinary(fileName + ".graph");
    }

    inline void deserialize(const std::string& fileName) noexcept
    {
        IO::deserialize(fileName, edgeWeights);
        graph.readBinary(fileName + ".graph");
    }

    inline long long byteSize() const noexcept
    {
        long long result = Vector::byteSize(edgeWeights);
        result += graph.byteSize();
        return result;
    }

public:
    // edge id -> vector of durations
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> edgeWeights;

    TDDGraph graph;
};

} // namespace RAPTOR
