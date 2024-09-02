#pragma once

#include <vector>

#include "../../DataStructures/PTL/Data.h"
#include "Profiler.h"

namespace PTL {

template <typename PROFILER = NoProfiler>
class Query {
public:
    using Profiler = PROFILER;

    Query(Data& data)
        : data(data)
    {
        profiler.registerPhases({ PHASE_FIND_FIRST_VERTEX,
            PHASE_INSERT_HASH,
            PHASE_RUN });
        profiler.registerMetrics({ METRIC_INSERTED_HUBS,
            METRIC_CHECK_ARR_EVENTS,
            METRIC_CHECK_HUBS,
            METRIC_FOUND_SOLUTIONS });
    };

    int run(const StopId source, const int departureTime, const StopId target) noexcept
    {
        AssertMsg(data.teData.isStop(source), "Source is not valid!");
        AssertMsg(data.teData.isStop(target), "Target is not valid!");
        AssertMsg(0 <= departureTime, "Time is negative!");

        profiler.start();

        profiler.startPhase();
        prepareStartingVertex(source, departureTime);
        profiler.donePhase(PHASE_FIND_FIRST_VERTEX);

        profiler.startPhase();
        prepareSet();
        profiler.donePhase(PHASE_INSERT_HASH);

        profiler.startPhase();

        const auto& arrEvents = data.teData.getArrivalsOfStop(target);

        size_t left = getIndexOfFirstEventAfterTime(arrEvents, departureTime);

        int finalTime = scanHubs(arrEvents, left);

        profiler.donePhase(PHASE_RUN);
        profiler.done();

        return finalTime;
    }

    inline bool prepareStartingVertex(const StopId stop, const int time) noexcept
    {
        Vertex firstReachableNode = data.teData.getFirstReachableDepartureVertexAtStop(stop, time);

        // Did we reach any transfer node?
        if (!data.teData.isEvent(firstReachableNode)) {
            return false;
        }

        AssertMsg(data.teData.isEvent(firstReachableNode), "First reachable node is not valid!");
        startingVertex = firstReachableNode;
        return true;
    }

    inline void prepareSet() noexcept
    {
        AssertMsg(data.teData.isEvent(startingVertex), "First reachable node is not valid!");

        hash.clear();

        for (auto& fwdHub : data.getFwdHubs(startingVertex)) {
            hash.insert(fwdHub);

            profiler.countMetric(METRIC_INSERTED_HUBS);
        }
    }

    inline size_t getIndexOfFirstEventAfterTime(const auto& arrEvents, const int time) noexcept
    {
        auto it = std::lower_bound(arrEvents.begin(), arrEvents.end(), time, [&](const size_t event, const int time) {
            return data.teData.getTimeOfVertex(Vertex(event)) < time;
        });

        return std::distance(arrEvents.begin(), it);
    }

    inline int scanHubs(const auto& arrEvents, const size_t left = 0) noexcept
    {
        for (size_t i = left; i < arrEvents.size(); ++i) {
            const auto& arrEventAtTarget = arrEvents[i];

            int arrTime = data.teData.getTimeOfVertex(Vertex(arrEventAtTarget));

            profiler.countMetric(METRIC_CHECK_ARR_EVENTS);

            const auto& bwdLabels = data.getBwdHubs(Vertex(arrEventAtTarget));

            for (const auto& hub : bwdLabels) {
                profiler.countMetric(METRIC_CHECK_HUBS);

                if (hash.find(hub) != hash.end()) [[unlikely]] {
                    profiler.countMetric(METRIC_FOUND_SOLUTIONS);
                    return arrTime;
                }
            }
        }
        return -1;
    }

    inline const Profiler& getProfiler() const noexcept
    {
        return profiler;
    }

    Data& data;
    Vertex startingVertex;
    std::set<Vertex> hash;
    Profiler profiler;
};
}
