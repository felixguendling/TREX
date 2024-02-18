#pragma once

// Builder for Number Of Cells = 2 on all levels

#include "../../../DataStructures/MLTB/MLData.h"
#include "../../../Helpers/Console/Progress.h"
#include "../../../Helpers/MultiThreading.h"
#include "../../../Helpers/String/String.h"
#include "../../TripBased/Query/Profiler.h"
#include "TransferSearchIBEs.h"
#include <cmath>
#include <execution>
#include <omp.h>
#include <vector>

namespace TripBased {
// IBE <=> Incomming Border Event
// Idea: use the lowest 8 bit for the stopindex, then the rest for the tripid
typedef uint32_t PackedIBE;

// Bit mask for the PackedIBE
constexpr uint32_t TRIPOFFSET = 8;

// Bit mask for the PackedIBE
constexpr uint32_t STOPINDEX_MASK = ((1 << 8) - 1);

class Builder {
public:
    Builder(MLData& data)
        : data(data)
        , search(data)
        , IBEs()
    {
    }

    inline void collectAllIBEsOnLowestLevel() noexcept
    {
        /* profiler.startPhase(); */
        IBEs.reserve(data.numberOfStopEvents());

        auto inSameCell = [&](auto a, auto b) {
            return (data.getCellIdOfStop(a) == data.getCellIdOfStop(b));
        };

        for (StopId stop(0); stop < data.numberOfStops(); ++stop) {
            for (const RAPTOR::RouteSegment& route : data.routesContainingStop(stop)) {
                // EDGE CASE: a stop is not a border stop (of a route) if it's at either end (start or end)
                if (route.stopIndex == 0)
                    continue;

                // check if the next / previous stop in stop array of route is in another cell
                RAPTOR::RouteSegment neighbourSeg(route.routeId, StopIndex(route.stopIndex - 1));

                // check if neighbour is *not* in the same cell
                if (!inSameCell(stop, data.raptorData.stopOfRouteSegment(neighbourSeg))) {
                    // add all stop events of this route
                    for (TripId trip : data.tripsOfRoute(route.routeId)) {
                        /* profiler.countMetric(COLLECTED_STOPEVENTS); */
                        IBEs.push_back((trip << TRIPOFFSET) | (route.stopIndex - 1));
                    }
                }
            }
        }
        /* profiler.donePhase(COLLECT_STOPEVENTS); */
    }

    inline void filterIrrelevantIBEs(uint8_t level)
    {
        // 'level' denotes the level, for which we filter the IBEs
        // ... in other word: after this method, IBEs should contain all IBEs which cross at this level, not lower levels

        IBEs.erase(
            std::remove_if(std::execution::par, IBEs.begin(), IBEs.end(), [&](PackedIBE ibe) {
                auto trip = TripId(ibe >> TRIPOFFSET);
                auto stopIndex = StopIndex(ibe & STOPINDEX_MASK);

                auto fromStop = data.getStop(trip, stopIndex);
                auto toStop = data.getStop(trip, StopIndex(stopIndex + 1));
                return !((data.getCellIdOfStop(fromStop) ^ data.getCellIdOfStop(toStop)) >> level);
            }),
            IBEs.end());
    }

    template <bool SORT_IBES = true, bool VERBOSE = true>
    inline void run() noexcept
    {
        collectAllIBEsOnLowestLevel();

        if (SORT_IBES)
            std::sort(std::execution::par, IBEs.begin(), IBEs.end());

        // now for every level, we have an invariant: IBEs contains exactly the IBEs we need on this level
        for (uint8_t level(0); level < data.getNumberOfLevels(); ++level) {
            if (VERBOSE)
                std::cout << "Starting Level " << (int)level << " [IBEs: " << IBEs.size() << "]... " << std::endl;

            Progress progress(IBEs.size());

            for (size_t i = 0; i < IBEs.size(); ++i) {
                auto& values = IBEs[i];

                search.run(TripId(values >> TRIPOFFSET), StopIndex(values & STOPINDEX_MASK), level);

                ++progress;
            }

            progress.finished();

            if (level < data.getNumberOfLevels() - 1) {
                filterIrrelevantIBEs(level + 1);
            }

            if (VERBOSE) {
                std::cout << "done!\n***** Stats *****\n";
                search.getProfiler().printStatisticsAsCSV();
            }
        }
    }

    template <bool SORT_IBES = true, bool VERBOSE = true>
    inline void run(const int numberOfThreads, const int pinMultiplier = 1) noexcept
    {
        collectAllIBEsOnLowestLevel();

        if (SORT_IBES)
            std::sort(std::execution::par, IBEs.begin(), IBEs.end());

        // TODO can be optimised
        std::vector<TransferSearch<TripBased::AggregateProfiler>> seekers;

        seekers.reserve(numberOfThreads);

        for (int i = 0; i < numberOfThreads; ++i)
            seekers.emplace_back(data);

        const int numCores = numberOfCores();
        omp_set_num_threads(numberOfThreads);

        // now for every level, we have an invariant: IBEs contains exactly the IBEs we need on this level
        for (uint8_t level(0); level < data.getNumberOfLevels(); ++level) {
            if (VERBOSE)
                std::cout << "Starting Level " << (int)level << " [IBEs: " << IBEs.size() << "]... " << std::endl;

            Progress progress(IBEs.size());

#pragma omp parallel
            {
#pragma omp for
                for (size_t i = 0; i < IBEs.size(); ++i) {
                    int threadId = omp_get_thread_num();
                    pinThreadToCoreId((threadId * pinMultiplier) % numCores);
                    AssertMsg(omp_get_num_threads() == numberOfThreads,
                        "Number of threads is " << omp_get_num_threads() << ", but should be " << numberOfThreads << "!");

                    auto& values = IBEs[i];
                    seekers[threadId].run(TripId(values >> TRIPOFFSET), StopIndex(values & STOPINDEX_MASK), level);
                    ++progress;
                }
            }

            progress.finished();

            if (level < data.getNumberOfLevels() - 1)
                filterIrrelevantIBEs(level + 1);

            if (VERBOSE)
                std::cout << "done!\n";
        }
    }

    MLData& data;
    TransferSearch<TripBased::AggregateProfiler> search;
    std::vector<PackedIBE> IBEs;
};
}
