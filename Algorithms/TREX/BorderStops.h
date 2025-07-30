#pragma once

#include <omp.h>

#include <iostream>
#include <vector>

#include "../../DataStructures/TREX/TREXData.h"
#include "../../Helpers/Console/Progress.h"

namespace TripBased {

class BorderStops {
 public:
  BorderStops(TREXData &data) : data(data), borderStops() {}

  void collectBorderStops() {
    borderStops.clear();
    borderStops.reserve(data.numberOfStops());

    auto inSameCell = [&](auto a, auto b) {
      return (data.getCellIdOfStop(a) == data.getCellIdOfStop(b));
    };

    for (StopId stop(0); stop < data.numberOfStops(); ++stop) {
      for (const RAPTOR::RouteSegment &route :
           data.routesContainingStop(stop)) {
        // EDGE CASE: a stop is not a border stop (of a route) if it's at either
        // end (start or end)
        if (route.stopIndex == 0) continue;

        // check if the next / previous stop in stop array of route is in
        // another cell
        RAPTOR::RouteSegment prevSeg(route.routeId,
                                     StopIndex(route.stopIndex - 1));

        // check if neighbour is *not* in the same cell
        if (!inSameCell(stop, data.raptorData.stopOfRouteSegment(prevSeg))) {
          borderStops.push_back(stop);
        }
      }
    }

    for (auto s : borderStops) {
      std::cout << "StopId: " << (int)s << std::endl;
    }
  }

  std::pair<std::vector<std::pair<TripId, StopIndex>>,
            std::vector<std::pair<TripId, StopIndex>>>
  collectIncommingAndOutgoingTrips(const int level, const int cell) {
    std::pair<std::vector<std::pair<TripId, StopIndex>>,
              std::vector<std::pair<TripId, StopIndex>>>
        result;
    result.first.reserve(2000);
    result.second.reserve(2000);

    auto inSameCell = [&](auto a, auto b, const int level) -> bool {
      assert(level >= 0 && level < 16);
      return (data.getCellIdOfStop(a) >> level) ==
             (data.getCellIdOfStop(b) >> level);
    };

    auto isInCell = [&](const StopId stop, const int level,
                        const int cell) -> bool {
      assert(level >= 0 && level < 16);
      return (data.getCellIdOfStop(stop) >> level) ==
             static_cast<std::uint16_t>(cell);
    };

    for (StopId stop(0); stop < data.numberOfStops(); ++stop) {
      if (!isInCell(stop, level, cell)) continue;
      for (const RAPTOR::RouteSegment &route :
           data.routesContainingStop(stop)) {
        if (route.stopIndex > 0) {
          RAPTOR::RouteSegment prevSeg(route.routeId,
                                       StopIndex(route.stopIndex - 1));

          if (!inSameCell(stop, data.raptorData.stopOfRouteSegment(prevSeg),
                          level)) {
            for (TripId trip : data.tripsOfRoute(route.routeId)) {
              result.first.emplace_back(trip, route.stopIndex);
            }
          }
        }
        if (route.stopIndex + 1 < data.numberOfStopsInRoute(route.routeId)) {
          RAPTOR::RouteSegment nextSeg(route.routeId,
                                       StopIndex(route.stopIndex + 1));

          if (!inSameCell(stop, data.raptorData.stopOfRouteSegment(nextSeg),
                          level)) {
            for (TripId trip : data.tripsOfRoute(route.routeId)) {
              result.second.emplace_back(trip, route.stopIndex);
            }
          }
        }
      }
    }

    return result;
  }

 private:
  TREXData &data;
  std::vector<StopId> borderStops;
};
}  // namespace TripBased
