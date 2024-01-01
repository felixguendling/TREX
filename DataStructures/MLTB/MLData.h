#pragma once

#include "../../Algorithms/UnionFind.h"
#include "../../Helpers/Assert.h"
#include "../../Helpers/Ranges/SubRange.h"
#include "../../Helpers/String/String.h"
/* #include "../Partition/MultiLevelCell.h" */
#include "../Partition/MultiLevelPartitionOLD.h"
#include "../RAPTOR/Data.h"
#include "../RAPTOR/Entities/RouteSegment.h"
#include "../TripBased/Data.h"
#include <numeric>
#include <string>

namespace TripBased {

class MLData : public Data {
public:
    MLData(const RAPTOR::Data& raptor, const int numLevels, const int numCellsPerLevel)
        : Data(raptor)
        , multiPartition(numberOfStops(), numLevels, numCellsPerLevel)
        , unionFind(numberOfStops())
        , layoutGraph()
        , localLevelOfEvent(raptor.numberOfStopEvents(), 0)
    {
    }

    MLData(const std::string& fileName)
    {
        deserialize(fileName);
    }

public:
    inline void addFlagsToStopEventGraph() noexcept
    {
        std::vector<uint8_t> zeroLevels(stopEventGraph.numEdges(), 0);
        stopEventGraph.get(LocalLevel).swap(zeroLevels);

        /* for (const auto [edge, from] : stopEventGraph.edgesWithFromVertex()) { */
        /*     StopEventId toVertex = StopEventId(stopEventGraph.get(ToVertex, edge)); */
        /*     AssertMsg(from < numberOfStopEvents(), "Transfers comes from a non-valid stopevent!"); */
        /*     AssertMsg(toVertex < numberOfStopEvents(), "Transfers leads to a non-valid stopevent!"); */

        /*     StopId fromStop = getStopOfStopEvent(StopEventId(from)); */
        /*     StopId toStop = getStopOfStopEvent(toVertex); */

        /*     AssertMsg(multiPartition.inSameCell(fromStop, toStop), "Stops are not in the same cell!"); */

        /*     stopEventGraph.set(LocalLevel, edge, 0); */
        /* } */
    }

    inline void showCuts() noexcept
    {
        std::vector<size_t> cutsPerLevel(multiPartition.getNumberOfLevels(), 0);

        for (const RouteId route : routes()) {
            SubRange<std::vector<StopId>> stopsInCurrentRoute = raptorData.stopsOfRoute(route);
            size_t numberOfTrips = raptorData.numberOfTripsInRoute(route);

            for (size_t i(1); i < stopsInCurrentRoute.size(); ++i) {
                if (stopsInCurrentRoute[i] == stopsInCurrentRoute[i - 1])
                    continue;

                size_t lowestCross = multiPartition.findCrossingLevel(stopsInCurrentRoute[i - 1], stopsInCurrentRoute[i]);

                if (lowestCross == (size_t)multiPartition.getNumberOfLevels())
                    continue;

                cutsPerLevel[lowestCross] += numberOfTrips;
                i = stopsInCurrentRoute.size();
            }
        }

        std::cout << "**** Information about the number of cuts depending on the level ****" << std::endl;
        std::cout << "**** Note: A trip can only be a cut trip once ****" << std::endl;
        std::cout << "Levels\tCut\tPercentage[%]" << std::endl;

        for (size_t level(0); level < cutsPerLevel.size(); ++level) {
            std::cout << level << "\t" << String::prettyInt(cutsPerLevel[level]) << "\t" << String::prettyDouble((float)(cutsPerLevel[level] / (float)raptorData.numberOfTrips()) * 100) << std::endl;
        }
    }

    inline void readPartitionFile(std::string fileName)
    {
        std::vector<int> globalIds(numberOfStops(), 0);
        std::fstream file(fileName);

        if (file.is_open()) {
            int globalId(0);
            size_t index(0);

            while (file >> globalId) {
                globalIds[index] = globalId;
                ++index;
            }

            file.close();

            std::cout << "Read " << String::prettyInt(index) << " many IDs!" << std::endl;
        } else {
            std::cerr << "Unable to open the file: " << fileName << std::endl;
        }

        for (size_t i(0); i < numberOfStops(); ++i) {
            AssertMsg((size_t)unionFind(i) < globalIds.size(), "unionFind[i] is out of bounds!");

            multiPartition.set(i, (size_t)globalIds[unionFind(i)]);
        }

        AssertMsg(assertNoCutTransfers(), "Footpath has been cut!");
    }

    inline void createCompactLayoutGraph()
    {
        std::cout << "Computing the Compact Layout Graph!" << std::endl;

        unionFind.clear();
        std::vector<int> weightOfNodes(numberOfStops(), 1);

        for (const auto [edge, from] : raptorData.transferGraph.edgesWithFromVertex()) {
            Vertex toStop = raptorData.transferGraph.get(ToVertex, edge);
            if (unionFind(from) != unionFind(toStop)) {
                weightOfNodes[unionFind(from)] += weightOfNodes[unionFind(toStop)];
                unionFind(from, toStop);
            }
        }

        DynamicGraphWithWeightsAndCoordinates dynamicLayoutGraph;
        dynamicLayoutGraph.clear();
        dynamicLayoutGraph.addVertices(numberOfStops());

        for (Vertex vertex : dynamicLayoutGraph.vertices()) {
            dynamicLayoutGraph.set(Weight, vertex, 0);
            if (unionFind(vertex) == (int)vertex) {
                dynamicLayoutGraph.set(Weight, vertex, weightOfNodes[vertex]);
            }
            dynamicLayoutGraph.set(Coordinates, vertex, raptorData.stopData[vertex].coordinates);
        }

        Progress progress(raptorData.numberOfRoutes() + raptorData.transferGraph.numEdges());

        for (const RouteId route : raptorData.routes()) {
            SubRange<std::vector<StopId>> stopsInCurrentRoute = raptorData.stopsOfRoute(route);
            size_t numberOfTrips = raptorData.numberOfTripsInRoute(route);

            for (size_t i(1); i < stopsInCurrentRoute.size(); ++i) {
                AssertMsg(dynamicLayoutGraph.isVertex(stopsInCurrentRoute[i]), "Current Stop is not a valid vertex!\n");
                Vertex fromVertexUnion = Vertex(unionFind(stopsInCurrentRoute[i - 1]));
                Vertex toVertexUnion = Vertex(unionFind(stopsInCurrentRoute[i]));

                if (fromVertexUnion == toVertexUnion)
                    continue;

                Edge edgeHeadTail = dynamicLayoutGraph.findEdge(fromVertexUnion, toVertexUnion);
                if (edgeHeadTail != noEdge) {
                    dynamicLayoutGraph.set(Weight, edgeHeadTail, dynamicLayoutGraph.get(Weight, edgeHeadTail) + numberOfTrips);
                    Edge edgeTailHead = dynamicLayoutGraph.findEdge(toVertexUnion, fromVertexUnion);
                    AssertMsg(edgeTailHead != noEdge, "A reverse edge is missing between " << stopsInCurrentRoute[i - 1] << " and " << stopsInCurrentRoute[i] << "\n");
                    dynamicLayoutGraph.set(Weight, edgeTailHead, dynamicLayoutGraph.get(Weight, edgeTailHead) + numberOfTrips);

                } else {
                    dynamicLayoutGraph.addEdge(fromVertexUnion, toVertexUnion)
                        .set(Weight, 1);
                    dynamicLayoutGraph.addEdge(toVertexUnion, fromVertexUnion)
                        .set(Weight, 1);
                }
            }
            ++progress;
        }

        progress.finished();

        AssertMsg(!(dynamicLayoutGraph.edges().size() & 1), "The number of edges is uneven, thus we check that every edge "
                                                            "has a reverse edge in the graph!\n");

        uint64_t totalEdgeWeight(0);
        for (Edge edge : dynamicLayoutGraph.edges()) {
            totalEdgeWeight += dynamicLayoutGraph.get(Weight, edge);
        }

        if (totalEdgeWeight > UINT32_MAX)
            std::cout << "** The total sum of all edge weights exceeds 32 bits **" << std::endl;

        dynamicLayoutGraph.packEdges();
        layoutGraph.clear();
        Graph::move(std::move(dynamicLayoutGraph), layoutGraph);
        std::cout << "The Layout Graph looks like this:" << std::endl;
        layoutGraph.printAnalysis();
    }

    inline void writeLayoutGraphToMETIS(const std::string fileName, const bool writeGRAPHML = true)
    {
        Progress progressWriting(layoutGraph.numVertices());

        unsigned long n = layoutGraph.numVertices();
        unsigned long m = layoutGraph.numEdges() >> 1; // halbieren weil METIS das braucht

        std::ofstream file(fileName + ".metis");

        // n [NUMBER of nodes]  m [NUMBER of edges]     f [int]
        // f values:
        /*
                f values:
        1 :     edge-weighted graph
        10:     node-weighted graph
        11:     edge & node - weighted graph
         */

        file << n << " " << m << " 11";

        for (Vertex vertex : layoutGraph.vertices()) {
            file << "\n"
                 << layoutGraph.get(Weight, vertex) << " ";
            for (Edge edge : layoutGraph.edgesFrom(vertex)) {
                file << layoutGraph.get(ToVertex, edge).value() + 1 << " " << layoutGraph.get(Weight, edge) << " ";
            }
            progressWriting++;
        }

        file.close();
        progressWriting.finished();

        if (writeGRAPHML) {
            Graph::toGML(fileName, layoutGraph);
        }

        layoutGraph.writeBinary(fileName);
    }

    // Getter
    inline int numberOfLevels() const noexcept
    {
        return multiPartition.getNumberOfLevels();
    }

    inline int numberOfCellsPerLevel() const noexcept
    {
        return multiPartition.getNumberOfCellsPerLevel();
    }

    inline SubRange<std::vector<RAPTOR::RouteSegment>> routesContainingStop(const StopId stop) const noexcept
    {
        return raptorData.routesContainingStop(stop);
    }

    inline bool stopInCell(StopId stop, std::vector<int> levels, std::vector<int> cellIds)
    {
        AssertMsg(isStop(stop), "Stop is not a stop!");
        return multiPartition.inSameCell(stop, levels, cellIds);
    }

    inline std::vector<int> getIdsOfStop(StopId stop)
    {
        AssertMsg(isStop(stop), "Stop is not a stop!");
        return multiPartition.getCellIds(stop);
    }

    inline uint8_t& getLocalLevelOfEvent(StopEventId event) noexcept
    {
        AssertMsg(event < localLevelOfEvent.size(), "Event is out of bounds!");

        return localLevelOfEvent[event];
    }

    inline std::vector<StopEventId> getStopEventOfStopInRoute(const StopId stop, const RouteId route) noexcept
    {
        std::vector<StopEventId> result;
        result.reserve(raptorData.numberOfTripsInRoute(route));

        auto stopsOfRoute = raptorData.stopsOfRoute(route);
        Range<TripId> tripsOfRoute = Data::tripsOfRoute(route);

        for (size_t i(0); i < stopsOfRoute.size(); ++i) {
            if (stopsOfRoute[i] == stop) {
                for (TripId trip : tripsOfRoute) {
                    result.push_back(getStopEventId(trip, StopIndex(i)));
                }
                return result;
            }
        }
        AssertMsg(false, "Wait, stop was not in the route??");
        return result;
    }

    template <int DIRECTION = -1>
    inline std::vector<std::pair<TripId, StopIndex>> getBorderStopEvents(std::vector<int> levels, std::vector<int> ids)
    {
        AssertMsg(levels.size() == ids.size(), "Levels and IDs need to be the same size!");
        std::vector<std::pair<TripId, StopIndex>> result;
        result.reserve(2000); // TODO pay attention to reserve

        std::vector<int> stopsInCell = multiPartition.verticesInCell(levels, ids);

        for (size_t i(0); i < stopsInCell.size(); ++i) {
            StopId stop(stopsInCell[i]);

            for (const RAPTOR::RouteSegment& route : routesContainingStop(stop)) {
                // EDGE CASE: a stop is not a border stop (of a route) if it's at either end (start or end)
                // boundary check for first / last stop on route
                if ((route.stopIndex == 0 && DIRECTION == -1) || (route.stopIndex == raptorData.numberOfStopsInRoute(route.routeId) - 1 && DIRECTION == 1))
                    continue;

                // check if the next / previous stop in stop array of route is in another cell
                RAPTOR::RouteSegment neighbourSeg(route.routeId, StopIndex(route.stopIndex + DIRECTION));

                // check if neighbour in same cell
                if (!multiPartition.inSameCell(raptorData.stopOfRouteSegment(neighbourSeg), levels, ids)) {
                    // add all stop events of this route (plus the DIRECTION we are facing)
                    for (TripId trip : tripsOfRoute(route.routeId)) {
                        result.push_back(std::make_pair(trip, StopIndex(route.stopIndex + DIRECTION)));
                    }
                }
            }
        }

        return result;
    }

    inline int getLowestCommonLevel(StopId a, StopId b)
    {
        return multiPartition.getLowestCommonLevel(a, b);
    }

    inline void printInfo() const noexcept
    {
        Data::printInfo();
        std::cout << "   Number of Levels:         " << std::setw(12) << multiPartition.getNumberOfLevels() << std::endl;
        std::cout << "   Cells per Level:          " << std::setw(12) << multiPartition.getNumberOfCellsPerLevel() << std::endl;
    }

    // Serialization

    inline void serialize(const std::string& fileName) const noexcept
    {
        Data::serialize(fileName + ".trip");
        IO::serialize(fileName, multiPartition, unionFind, layoutGraph, localLevelOfEvent);
        stopEventGraph.writeBinary(fileName + ".trip.graph");
    }

    inline void deserialize(const std::string& fileName) noexcept
    {
        Data::deserialize(fileName + ".trip");
        IO::deserialize(fileName, multiPartition, unionFind, layoutGraph, localLevelOfEvent);
        stopEventGraph.readBinary(fileName + ".trip.graph");
    }

    // Assert that no transfer is cut
    inline bool assertNoCutTransfers() noexcept
    {
        for (const auto [edge, from] : raptorData.transferGraph.edgesWithFromVertex()) {
            Vertex toVertex = raptorData.transferGraph.get(ToVertex, edge);
            if (!multiPartition.inSameCell(from, toVertex)) {
                std::cout << "*** A cut between footpath from " << from << " and " << toVertex << "! The respective union find: " << unionFind(from) << " and " << unionFind(toVertex) << std::endl;
                return false;
            }
        }
        return true;
    }

    inline bool isLevel(size_t level) const
    {
        return multiPartition.isLevelValid(level);
    }

    inline bool isCell(size_t level, size_t cell) const
    {
        return cell < multiPartition.getNumberOfCellsInLevel(level);
    }

    inline void writePartitionToCSV(const std::string& fileName) noexcept
    {
        std::ofstream file(fileName);

        file << "StopID";

        for (size_t level(0); level < multiPartition.getNumberOfLevels(); ++level)
            file << ",Level " << level;
        file << "\n";

        auto& cells = multiPartition.getIds();
        for (size_t i(0); i < cells.size(); ++i) {
            file << i;
            for (size_t l(0); l < multiPartition.getNumberOfLevels(); ++l)
                file << "," << cells[i][l];
            file << "\n";
        }
        file.close();
    }

public:
    MultiLevelPartition multiPartition;
    UnionFind unionFind;
    StaticGraphWithWeightsAndCoordinates layoutGraph;

    // we also keep track of the highest locallevel of an event
    std::vector<uint8_t> localLevelOfEvent;
};

} // namespace TripBased
