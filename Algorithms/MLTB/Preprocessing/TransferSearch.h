#pragma once

#include "../../../DataStructures/Container/Set.h"
#include "../../../DataStructures/MLTB/MLData.h"
#include "../../../DataStructures/RAPTOR/Entities/ArrivalLabel.h"
#include "../../../DataStructures/RAPTOR/Entities/Journey.h"
#include "../../../DataStructures/TripBased/Data.h"
#include "../../TripBased/Query/Profiler.h"
#include "../../TripBased/Query/ReachedIndex.h"
/* #include "ReachedIndex.h" */

namespace TripBased {

template <typename PROFILER = NoProfiler>
class TransferSearch {
public:
    using Profiler = PROFILER;
    using Type = TransferSearch<Profiler>;

private:
    struct TripLabel {
        TripLabel(const StopEventId begin = noStopEvent, const StopEventId end = noStopEvent,
            const u_int32_t parent = -1, const Edge parentTransfer = noEdge)
            : begin(begin)
            , end(end)
            , parent(parent)
            , parentTransfer(parentTransfer)
        {
        }
        StopEventId begin;
        StopEventId end;
        u_int32_t parent;
        Edge parentTransfer;
    };

    struct EdgeRange {
        EdgeRange()
            : begin(noEdge)
            , end(noEdge)
        {
        }

        Edge begin;
        Edge end;
    };

    struct EdgeLabel {
        EdgeLabel(const StopEventId stopEvent = noStopEvent, const TripId trip = noTripId,
            const StopEventId firstEvent = noStopEvent)
            : stopEvent(stopEvent)
            , trip(trip)
            , firstEvent(firstEvent)
        {
        }

        StopEventId stopEvent;
        TripId trip;
        StopEventId firstEvent;
    };

    struct RouteLabel {
        RouteLabel()
            : numberOfTrips(0)
        {
        }
        inline StopIndex end() const noexcept
        {
            return StopIndex(departureTimes.size() / numberOfTrips);
        }
        u_int32_t numberOfTrips;
        std::vector<int> departureTimes;
    };

public:
    TransferSearch(MLData& data)
        : data(data)
        , queue(data.numberOfStopEvents())
        , edgeRanges(data.numberOfStopEvents())
        , queueSize(0)
        , reachedIndex(data)
        , edgeLabels(data.stopEventGraph.numEdges())
        , routeLabels(data.numberOfRoutes())
        , localLevels(data.stopEventGraph.numEdges(), 0)
        , toBeUnpacked(data.numberOfStopEvents())
        , extractedPaths(0)
        , totalLengthPfExtractedPaths(0)
    {
        for (const Edge edge : data.stopEventGraph.edges()) {
            edgeLabels[edge].stopEvent = StopEventId(data.stopEventGraph.get(ToVertex, edge) + 1);
            edgeLabels[edge].trip = data.tripOfStopEvent[data.stopEventGraph.get(ToVertex, edge)];
            edgeLabels[edge].firstEvent = data.firstStopEventOfTrip[edgeLabels[edge].trip];
        }

        for (const RouteId route : data.raptorData.routes()) {
            const size_t numberOfStops = data.numberOfStopsInRoute(route);
            const size_t numberOfTrips = data.raptorData.numberOfTripsInRoute(route);
            const RAPTOR::StopEvent* stopEvents = data.raptorData.firstTripOfRoute(route);
            routeLabels[route].numberOfTrips = numberOfTrips;
            routeLabels[route].departureTimes.resize((numberOfStops - 1) * numberOfTrips);
            for (size_t trip = 0; trip < numberOfTrips; trip++) {
                for (size_t stopIndex = 0; stopIndex + 1 < numberOfStops; stopIndex++) {
                    routeLabels[route].departureTimes[(stopIndex * numberOfTrips) + trip] = stopEvents[(trip * numberOfStops) + stopIndex].departureTime;
                }
            }
        }
        profiler.registerPhases({ PHASE_SCAN_TRIPS });
        profiler.registerMetrics({ METRIC_ROUNDS, METRIC_SCANNED_TRIPS, METRIC_SCANNED_STOPS, METRIC_RELAXED_TRANSFERS,
            METRIC_ENQUEUES });
    }

    inline void run(const TripId trip, const StopIndex stopIndex, std::vector<int> currentLevels, std::vector<int> currentCellIds) noexcept
    {
        AssertMsg(data.isTrip(trip), "Trip is not valid!");
        AssertMsg(stopIndex < data.numberOfStopsInTrip(trip), "StopIndex is not valid!");

        profiler.start();
        clear();
        levels = currentLevels;
        cellIds = currentCellIds;
        minLevel = *std::min_element(levels.begin(), levels.end());

        enqueue(trip, stopIndex);
        scanTrips();
        unpack();
        profiler.done();
    }

    inline Profiler& getProfiler() noexcept
    {
        return profiler;
    }

    inline std::vector<uint8_t>& getLocalLevels() noexcept
    {
        return localLevels;
    }

private:
    inline void clear() noexcept
    {
        queueSize = 0;
        reachedIndex.clear();
        toBeUnpacked.clear();
    }

    inline void scanTrips() noexcept
    {
        profiler.startPhase();
        u_int8_t currentRoundNumber = 0;
        size_t roundBegin = 0;
        size_t roundEnd = queueSize;
        while (roundBegin < roundEnd && currentRoundNumber < 15) {
            ++currentRoundNumber;
            profiler.countMetric(METRIC_ROUNDS);

            for (size_t i = roundBegin; i < roundEnd; i++) {
                const TripLabel& label = queue[i];
                profiler.countMetric(METRIC_SCANNED_TRIPS);
                for (StopEventId j = label.begin; j < label.end; j++) {
                    profiler.countMetric(METRIC_SCANNED_STOPS);
                    // check if stop of j is outside this cell => add to 'toBeUnpacked'
                    StopId currentStop = data.getStopOfStopEvent(j);
                    if (!isStopInCell(currentStop)) {
                        toBeUnpacked.insert(i);
                    }
                }
            }

            for (size_t i = roundBegin; i < roundEnd; i++) {
                TripLabel& label = queue[i];
                edgeRanges[i].begin = data.stopEventGraph.beginEdgeFrom(Vertex(label.begin));
                edgeRanges[i].end = data.stopEventGraph.beginEdgeFrom(Vertex(label.end));
            }

            for (size_t i = roundBegin; i < roundEnd; i++) {
                const EdgeRange& label = edgeRanges[i];

                for (Edge edge = label.begin; edge < label.end; edge++) {
                    profiler.countMetric(METRIC_RELAXED_TRANSFERS);
                    enqueue(edge, i);
                }
            }
            roundBegin = roundEnd;
            roundEnd = queueSize;
        }
        profiler.donePhase(PHASE_SCAN_TRIPS);
    }

    inline bool isStopInCell(StopId stop) const
    {
        AssertMsg(data.isStop(stop), "Stop is not a valid stop!");
        return data.stopInCell(stop, levels, cellIds);
    }

    inline void enqueue(const TripId trip, const StopIndex index) noexcept
    {
        profiler.countMetric(METRIC_ENQUEUES);
        if (reachedIndex.alreadyReached(trip, index))
            return;
        const StopEventId firstEvent = data.firstStopEventOfTrip[trip];
        queue[queueSize] = TripLabel(StopEventId(firstEvent + index), StopEventId(firstEvent + reachedIndex(trip)));
        queueSize++;
        AssertMsg(queueSize <= queue.size(), "Queue is overfull!");
        reachedIndex.update(trip, index);
    }

    inline void enqueue(const Edge edge, const size_t parent) noexcept
    {
        profiler.countMetric(METRIC_ENQUEUES);
        const EdgeLabel& label = edgeLabels[edge];

        // break if a) already reached OR b) the stop if this transfer is not in the same cell
        if (reachedIndex.alreadyReached(label.trip, label.stopEvent - label.firstEvent) || !isStopInCell(data.getStop(label.trip, StopIndex(label.stopEvent - label.firstEvent - 1)))) [[likely]]
            return;

        if (minLevel > localLevels[edge])
            return;

        queue[queueSize] = TripLabel(
            label.stopEvent,
            StopEventId(label.firstEvent + reachedIndex(label.trip)),
            parent,
            edge);

        queueSize++;
        AssertMsg(queueSize <= queue.size(), "Queue is overfull!");
        reachedIndex.update(label.trip, StopIndex(label.stopEvent - label.firstEvent));
    }

    inline void unpack()
    {
        for (const size_t index : toBeUnpacked) {
            unpackStopEvent(index);

            ++extractedPaths;
        }
    }

    inline void unpackStopEvent(size_t index)
    {
        AssertMsg(index < queueSize, "Index is out of bounds!");

        TripLabel& label = queue[index];
        Edge currentEdge = label.parentTransfer;

        while (currentEdge != noEdge) {
            localLevels[currentEdge] = minLevel + 1;

            index = label.parent;
            label = queue[index];
            currentEdge = label.parentTransfer;

            ++totalLengthPfExtractedPaths;
        }
    }

public:
    inline double getAvgPathLengthPerLevel() noexcept
    {
        return (double)totalLengthPfExtractedPaths / (double)extractedPaths;
    }

    inline void resetStats() noexcept
    {
        totalLengthPfExtractedPaths = 0;
        extractedPaths = 0;
    }

private:
    MLData& data;

    std::vector<TripLabel> queue;
    std::vector<EdgeRange> edgeRanges;
    size_t queueSize;
    ReachedIndex reachedIndex;

    std::vector<EdgeLabel> edgeLabels;
    std::vector<RouteLabel> routeLabels;
    std::vector<uint8_t> localLevels;

    std::vector<int> levels;
    std::vector<int> cellIds;
    int minLevel;

    Profiler profiler;

    IndexedSet<false, size_t> toBeUnpacked;

    // stats
    uint64_t extractedPaths;
    uint64_t totalLengthPfExtractedPaths;
};

} // namespace TripBased
