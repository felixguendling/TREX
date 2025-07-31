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

#include "../../../DataStructures/TripBased/Data.h"
#include "../Query/ReachedIndex.h"

namespace TripBased {

class ShortcutAugmenter {
 public:
  struct RouteSegmentData {
    RouteSegmentData(const Edge edge, const TripId fromTrip,
                     const StopEventId toEvent)
        : edge(edge), fromTrip(fromTrip), toEvent(toEvent) {}

    Edge edge;
    TripId fromTrip;
    StopEventId toEvent;
  };

  ShortcutAugmenter() {}

  inline void augmentShortcuts(Data& data,
                               const size_t tripLimit = INFTY) noexcept {
    DynamicTransferGraph result;
    Graph::copy(data.stopEventGraph, result);
    result.deleteEdges([&](const Edge) { return true; });
    result.reserve(data.stopEventGraph.numVertices(),
                   data.stopEventGraph.numEdges() * 10);
    IndexedMap<RouteSegmentData, false, size_t> reachedRouteSegments(
        data.raptorData.numberOfRouteSegments());
    Progress progress(data.raptorData.numberOfRoutes());
    for (const RouteId fromRoute : data.raptorData.routes()) {
      progress++;
      for (StopIndex fromIndex(0);
           fromIndex < data.numberOfStopsInRoute(fromRoute); fromIndex++) {
        reachedRouteSegments.clear();
        for (TripId fromTrip(data.firstTripOfRoute[fromRoute + 1] - 1);
             fromTrip != data.firstTripOfRoute[fromRoute] - 1; fromTrip--) {
          const StopEventId fromStopEvent(data.firstStopEventOfTrip[fromTrip] +
                                          fromIndex);
          for (const Edge edge :
               data.stopEventGraph.edgesFrom(Vertex(fromStopEvent))) {
            const StopEventId toStopEvent(
                data.stopEventGraph.get(ToVertex, edge));
            const TripId toTrip = data.tripOfStopEvent[toStopEvent];
            const RouteId toRoute = data.routeOfTrip[toTrip];
            const StopIndex toIndex = data.indexOfStopEvent[toStopEvent];
            const size_t toSegment =
                data.raptorData.getRouteSegmentNum(toRoute, toIndex);
            if (reachedRouteSegments.contains(toSegment)) {
              const StopEventId oldToEvent =
                  reachedRouteSegments[toSegment].toEvent;
              if (toStopEvent <= oldToEvent) {
                reachedRouteSegments[toSegment] =
                    RouteSegmentData(edge, fromTrip, toStopEvent);
              }
            } else {
              reachedRouteSegments.insert(
                  toSegment, RouteSegmentData(edge, fromTrip, toStopEvent));
            }
          }
          for (const RouteSegmentData& d : reachedRouteSegments.getValues()) {
            if (d.fromTrip > fromTrip + tripLimit) continue;
            const Edge edge = d.edge;
            const Vertex toVertex = data.stopEventGraph.get(ToVertex, edge);
            const int travelTime = data.stopEventGraph.get(TravelTime, edge);
            result.addEdge(Vertex(fromStopEvent), toVertex)
                .set(TravelTime, travelTime);
          }
        }
      }
    }
    progress.finished();
    Graph::move(std::move(result), data.stopEventGraph);
  }

  inline void removeSuperfluousShortcuts(Data& data) noexcept {
    DynamicTransferGraph result;
    Graph::copy(data.stopEventGraph, result);
    result.deleteEdges([&](const Edge) { return true; });
    result.reserve(data.stopEventGraph.numVertices(),
                   data.stopEventGraph.numEdges());
    Progress progress(data.numberOfTrips());
    ReachedIndex reachedIndex(data);
    for (const TripId fromTrip : data.trips()) {
      progress++;
      reachedIndex.clear();
      for (StopEventId fromStopEvent(data.firstStopEventOfTrip[fromTrip + 1] -
                                     1);
           fromStopEvent != data.firstStopEventOfTrip[fromTrip] - 1;
           fromStopEvent--) {
        for (const Edge edge :
             data.stopEventGraph.edgesFrom(Vertex(fromStopEvent))) {
          const StopEventId toStopEvent(
              data.stopEventGraph.get(ToVertex, edge));
          const TripId toTrip = data.tripOfStopEvent[toStopEvent];
          const StopIndex toIndex = data.indexOfStopEvent[toStopEvent];
          if (reachedIndex(toTrip) > toIndex) {
            result.addEdge(Vertex(fromStopEvent), Vertex(toStopEvent),
                           data.stopEventGraph.edgeRecord(edge));
          }
          reachedIndex.update(toTrip, toIndex);
        }
      }
    }
    progress.finished();
    Graph::move(std::move(result), data.stopEventGraph);
  }
};

}  // namespace TripBased
