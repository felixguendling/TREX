#pragma once

#include <vector>

#include "../../DataStructures/TE/Data.h"
/* #include "../../Helpers/Vector/TimestampedVector.h" */
#include "../Dijkstra/Dijkstra.h"
#include "Profiler.h"

namespace TE {

template <typename PROFILER = NoProfiler, bool NODE_BLOCKING = false>
class Query {
public:
    using Profiler = PROFILER;

    Query(const Data& data)
        : data(data)
        , reachedTrip(data.numberOfRoutes(), TripId(data.numberOfTrips()))
        , dijkstra(data.timeExpandedGraph, TravelTime)
    {
        profiler.registerPhases({ PHASE_CLEAR, PHASE_FIND_FIRST_VERTEX, PHASE_RUN });
        profiler.registerMetrics({ METRIC_SEETLED_VERTICES,
            METRIC_RELAXED_EDGES,
            METRIC_FOUND_SOLUTIONS });
    };

    int run(const StopId source, const int departureTime, const StopId target) noexcept
    {
        profiler.start();

        AssertMsg(data.isStop(source), "Source is not valid!");
        AssertMsg(data.isStop(target), "Target is not valid!");
        AssertMsg(0 <= departureTime, "Time is negative!");

        profiler.startPhase();
        Vertex firstReachableNode = Vertex(data.getFirstReachableStopEventAtStop(source, departureTime));

        // Did we reach any transfer node?
        if (firstReachableNode == data.numberOfStopEvents()) {
            return -1;
        }

        AssertMsg(firstReachableNode < data.numberOfStopEvents(), "First reachable node " << firstReachableNode << " is not valid!");

        profiler.donePhase(PHASE_FIND_FIRST_VERTEX);

        profiler.startPhase();
        dijkstra.clear();

        if constexpr (NODE_BLOCKING) {
            std::fill(reachedTrip.begin(), reachedTrip.end(), TripId(data.numberOfTrips()));
        }
        profiler.donePhase(PHASE_CLEAR);

        profiler.startPhase();
        dijkstra.addSource(firstReachableNode, 0);

        auto targetPruning = [&]() {
            Vertex front = dijkstra.getQFront();
            return (data.timeExpandedGraph.get(StopVertex, front) == target);
        };

        auto settle = [&](const auto /* vertex */) {
            profiler.countMetric(METRIC_SEETLED_VERTICES);
        };

        auto pruneEdge = [&](const auto /* from */, [[maybe_unused]] const auto edge) {
            profiler.countMetric(METRIC_RELAXED_EDGES);

            if constexpr (NODE_BLOCKING) {
                Vertex toVertex = data.timeExpandedGraph.get(ToVertex, edge);
                if (toVertex < data.numberOfStopEvents())
                    return false;

                TripId toTrip = data.timeExpandedGraph.get(TripVertex, toVertex);

                if (toTrip == noTripId) return false;
                RouteId toRoute = data.routeOfTrip[toTrip];

                if (reachedTrip[toRoute] <= toTrip)
                    return true;

                reachedTrip[toRoute] = toTrip;
            }
            return false;
        };

        dijkstra.run(noVertex, settle, targetPruning, pruneEdge);
        profiler.donePhase(PHASE_RUN);

        // either the finalVertex is noVertex => no path found
        // or it has to be a transfer node at the target stop
        Vertex finalVertex = dijkstra.getQFront();
        AssertMsg(finalVertex == noVertex || data.timeExpandedGraph.get(StopVertex, finalVertex) == target, "last vertex was neither noVertex or at the target?");

        if (finalVertex != noVertex) {
            profiler.countMetric(METRIC_FOUND_SOLUTIONS);
        }

        profiler.done();

        return (finalVertex == noVertex ? -1 : dijkstra.getDistance(finalVertex));
    }

    inline const Profiler& getProfiler() const noexcept
    {
        return profiler;
    }

private:
    const Data& data;
    std::vector<TripId> reachedTrip;

    Dijkstra<TimeExpandedGraph> dijkstra;
    Profiler profiler;
};
}
