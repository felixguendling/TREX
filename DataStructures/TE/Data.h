#pragma once

#include <algorithm>
#include <unordered_map>

#include "../Graph/Graph.h"
#include "../Intermediate/Data.h"

#include "../../Helpers/Assert.h"
#include "../../Helpers/Console/Progress.h"
#include "../../Helpers/IO/Serialization.h"
#include "../../Helpers/Ranges/Range.h"
#include "../../Helpers/Ranges/SubRange.h"
#include "../../Helpers/String/Enumeration.h"
#include "../../Helpers/String/String.h"

namespace TE {

class Data {
public:
    Data() {};

    Data(const std::string& fileName)
    {
        deserialize(fileName);
    }

    inline static Data FromBinary(const std::string& fileName) noexcept
    {
        Data data;
        data.deserialize(fileName);
        return data;
    }

    inline static Data FromIntermediate(const Intermediate::Data& inter, const bool extractFootpaths = true) noexcept
    {
        return FromIntermediate(inter, inter.greedyfifoRoutes(), extractFootpaths);
    }

    inline static Data FromIntermediate(const Intermediate::Data& inter, const std::vector<std::vector<Intermediate::Trip>>& routes, const bool extractFootpaths = true) noexcept
    {
        Data data;
        for (const Intermediate::Stop& stop : inter.stops) {
            data.stopData.emplace_back(stop);
        }

        data.stopsOfRoute.assign(routes.size(), {});
        data.numTrips = 0;

        size_t numStopEvents = 0;

        for (RouteId i(0); i < routes.size(); ++i) {
            const auto& route = routes[i];
            AssertMsg(!route.empty(), "A route should not be empty!");

            numStopEvents += route[0].stopEvents.size() * route.size();
            data.numTrips += route.size();

            for (auto& event : route[0].stopEvents) {
                data.stopsOfRoute[i].emplace_back(event.stopId);
            }
        }

        data.stopEventData.reserve(numStopEvents);
        data.stopEventIdsOfStop.assign(data.stopData.size(), {});
        data.tripOfEvent.assign(numStopEvents, noTripId);

        DynamicTimeExpandedGraph builderGraph;
        size_t numberOfVerticesToAdd = numStopEvents * 3;

        builderGraph.addVertices(numberOfVerticesToAdd);

        builderGraph[RouteVertex].assign(numberOfVerticesToAdd, noRouteId);
        builderGraph[StopVertex].assign(numberOfVerticesToAdd, noStop);

        std::cout << "Adding the event vertices with all information like StopId, RouteId, ..." << std::endl;
        Progress progress(numStopEvents);
        Vertex currentVertex(0);
        TripId tripId(0);

        for (RouteId i = RouteId(0); i < routes.size(); i++) {
            const auto& route = routes[i];
            AssertMsg(!route.empty(), "A route should not be empty!");

            const auto& firstTrip = route[0];
            data.routeData.emplace_back(firstTrip.routeName, firstTrip.type);

            AssertMsg(!firstTrip.stopEvents.empty(), "Trip is empty??");
            for (const Intermediate::Trip& trip : route) {
                for (const Intermediate::StopEvent& event : trip.stopEvents) {
                    // set data
                    data.stopEventData.emplace_back(event);

                    data.stopEventIdsOfStop[event.stopId].emplace_back(currentVertex);
                    data.tripOfEvent[currentVertex] = tripId;

                    // transfer
                    builderGraph.set(StopVertex, data.getTransferVertexOfEvent(currentVertex), event.stopId);
                    builderGraph.set(StopVertex, data.getArrivalVertexOfEvent(currentVertex), event.stopId);
                    builderGraph.set(StopVertex, data.getDepartureVertexOfEvent(currentVertex), event.stopId);

                    // arr & dep
                    builderGraph.set(RouteVertex, data.getArrivalVertexOfEvent(currentVertex), i);
                    builderGraph.set(RouteVertex, data.getDepartureVertexOfEvent(currentVertex), i);

                    // add edges
                    builderGraph.addEdge(data.getArrivalVertexOfEvent(currentVertex), data.getDepartureVertexOfEvent(currentVertex)).set(TravelTime, event.departureTime - event.arrivalTime);
                    builderGraph.addEdge(data.getTransferVertexOfEvent(currentVertex), data.getDepartureVertexOfEvent(currentVertex)).set(TravelTime, 0);
                    builderGraph.addEdge(data.getArrivalVertexOfEvent(currentVertex), data.getTransferVertexOfEvent(currentVertex)).set(TravelTime, 0);

                    ++currentVertex;
                    ++progress;
                }
                ++tripId;
            }
        }
        progress.finished();
        std::cout << "Done!" << std::endl;

        AssertMsg(tripId == inter.numberOfTrips(), "Number of trips do not align, " << tripId << " vs " << inter.numberOfTrips() << "!");
        // add transfer edges and transfer chains

        // this is to keep track which stop events we already added to which stop
        std::unordered_map<size_t, size_t> reachableEvent(data.numberOfStops());

        std::cout << "Adding the transferchain " << (extractFootpaths ? "and the footpaths between transfer vertices " : "") << "..." << std::endl;
        Progress progress2(data.stopData.size());

        for (StopId stop = StopId(0); stop < data.stopData.size(); ++stop) {
            auto& values = data.stopEventIdsOfStop[stop];

            std::sort(values.begin(), values.end(), [&](size_t left, size_t right) {
                return data.getTimeOfVertex(Vertex(left), stop) < data.getTimeOfVertex(Vertex(right), stop);
            });

            int previousTime = data.getTimeOfVertex(data.getTransferVertexOfEvent(values[0]), stop);
            int currentTime = 0;

            // transfer chain
            for (size_t i = 1; i < values.size(); ++i) {
                currentTime = data.getTimeOfVertex(data.getTransferVertexOfEvent(values[i]), stop);
                AssertMsg(previousTime <= currentTime, "Time travel of transfer nodes!");
                builderGraph.addEdge(data.getTransferVertexOfEvent(values[i - 1]), data.getTransferVertexOfEvent(values[i])).set(TravelTime, currentTime - previousTime);

                previousTime = currentTime;
            }

            if (extractFootpaths) {
                for (const auto edge : inter.transferGraph.edgesFrom(stop)) {
                    reachableEvent[inter.transferGraph.get(ToVertex, edge)] = static_cast<size_t>(data.stopEventData.size());
                }

                // in reverse order, since we don't want to create any dubious duplicate footpaths
                for (size_t i = values.size(); i > 0; --i) {
                    for (const auto edge : inter.transferGraph.edgesFrom(stop)) {
                        Vertex toStop = inter.transferGraph.get(ToVertex, edge);

                        int timeAtStop = data.getTimeOfVertex(data.getTransferVertexOfEvent(values[i - 1]), stop) + inter.transferGraph.get(TravelTime, edge);

                        size_t earliestEvent = data.getFirstReachableStopEventAtStop(StopId(toStop), timeAtStop);

                        // no improvement?
                        if (earliestEvent >= reachableEvent[toStop]) {
                            continue;
                        }
                        AssertMsg(earliestEvent < data.stopEventData.size(), "Reached Event is not valid!");
                        reachableEvent[toStop] = earliestEvent;

                        // otherwise we can reach a better / earlier event, hence add the footpath
                        builderGraph.addEdge(data.getTransferVertexOfEvent(values[i - 1]), data.getTransferVertexOfEvent(earliestEvent)).set(TravelTime, inter.transferGraph.get(TravelTime, edge));
                    }
                }
                reachableEvent.clear();
            }

            ++progress2;
        }
        progress2.finished();
        std::cout << "Done!" << std::endl;

        builderGraph.sortEdges(ToVertex);

        Graph::move(std::move(builderGraph), data.timeExpandedGraph);

        return data;
    }

public:
    inline size_t numberOfStops() const noexcept { return stopData.size(); }
    inline bool isStop(const StopId stop) const noexcept { return stop < numberOfStops(); }
    inline Range<StopId> stops() const noexcept { return Range<StopId>(StopId(0), StopId(numberOfStops())); }

    inline size_t numberOfRoutes() const noexcept { return routeData.size(); }
    inline bool isRoute(const RouteId route) const noexcept { return route < numberOfRoutes(); }
    inline Range<RouteId> routes() const noexcept { return Range<RouteId>(RouteId(0), RouteId(numberOfRoutes())); }

    inline size_t numberOfTrips() const noexcept { return numTrips; }
    inline bool isTrip(const TripId route) const noexcept { return route < numberOfTrips(); }
    inline Range<TripId> trips() const noexcept { return Range<TripId>(TripId(0), TripId(numberOfTrips())); }

    inline size_t numberOfStopEvents() const noexcept { return stopEventData.size(); }

    inline size_t numberOfStopsInRoute(const RouteId route) const noexcept
    {
        AssertMsg(isRoute(route), "The id " << route << " does not represent a route!");
        return stopsOfRoute[route].size();
    }

    inline std::vector<StopId>& getStopsOfRoute(const RouteId route) noexcept
    {
        AssertMsg(isRoute(route), "The id " << route << " does not represent a route!");
        return stopsOfRoute[route];
    }

    inline Vertex getTransferVertexOfEvent(const size_t event) const noexcept
    {
        AssertMsg(event < numberOfStopEvents(), "Event is out of bounds!");
        return Vertex(event);
    }

    inline Vertex getArrivalVertexOfEvent(const size_t event) const noexcept
    {
        AssertMsg(event < numberOfStopEvents(), "Event is out of bounds!");
        return Vertex(numberOfStopEvents() + event);
    }

    inline Vertex getDepartureVertexOfEvent(const size_t event) const noexcept
    {
        AssertMsg(event < numberOfStopEvents(), "Event is out of bounds!");
        return Vertex(2 * numberOfStopEvents() + event);
    }

    inline int getTimeOfVertex(Vertex vertex, StopId stop) const noexcept
    {
        AssertMsg(vertex < 3 * numberOfStopEvents(), "Vertex " << vertex << " is not valid!");
        AssertMsg(isStop(stop), "Stop is not a stop!");

        // transfer node
        if (vertex < numberOfStopEvents()) {
            return stopEventData[vertex].arrivalTime + stopData[stop].minTransferTime;
        }

        vertex -= numberOfStopEvents();

        // arr node or dep node
        if (vertex < numberOfStopEvents()) {
            return stopEventData[vertex].arrivalTime;
        } else {
            return stopEventData[vertex].departureTime;
        }
    }

    inline size_t getFirstReachableStopEventAtStop(const StopId stop, const int time) const noexcept
    {
        AssertMsg(isStop(stop), "Stop is not a stop!");

        if (stopEventIdsOfStop[stop].empty()) {
            return stopEventData.size();
        }

        if (time < stopEventData[stopEventIdsOfStop[stop].front()].departureTime) {
            return stopEventData.size();
        }

        if (time > stopEventData[stopEventIdsOfStop[stop].back()].departureTime) {
            return stopEventData.size();
        }

        for (size_t id : stopEventIdsOfStop[stop]) {
            if (time <= stopEventData[id].departureTime) {
                return id;
            }
        }

        AssertMsg(false, "No event found?");
        return stopEventData.size();
    }

    inline void printInfo() const noexcept
    {
        std::cout << "TE public transit data:" << std::endl;
        std::cout << "   Number of Stops:          " << std::setw(12) << String::prettyInt(numberOfStops()) << std::endl;
        std::cout << "   Number of Trips:          " << std::setw(12) << String::prettyInt(numberOfTrips()) << std::endl;
        std::cout << "   Number of Routes:         " << std::setw(12) << String::prettyInt(numberOfRoutes()) << std::endl;
        std::cout << "   Number of Stop Events:    " << std::setw(12) << String::prettyInt(numberOfStopEvents()) << std::endl;
        std::cout << "   Number of TE Vertices:    " << std::setw(12) << String::prettyInt(timeExpandedGraph.numVertices()) << std::endl;
        std::cout << "   Number of TE Edges:       " << std::setw(12) << String::prettyInt(timeExpandedGraph.numEdges()) << std::endl;
    }

    inline void serialize(const std::string& fileName) const noexcept
    {
        IO::serialize(fileName, stopData, routeData, stopEventData, stopEventIdsOfStop, stopsOfRoute, tripOfEvent, numTrips);
        timeExpandedGraph.writeBinary(fileName + ".graph");
    }

    inline void deserialize(const std::string& fileName) noexcept
    {
        IO::deserialize(fileName, stopData, routeData, stopEventData, stopEventIdsOfStop, stopsOfRoute, tripOfEvent, numTrips);
        timeExpandedGraph.readBinary(fileName + ".graph");
    }

    inline long long byteSize() const noexcept
    {
        long long result = Vector::byteSize(stopData);
        result += Vector::byteSize(routeData);
        result += Vector::byteSize(stopEventData);
        result += Vector::byteSize(stopEventIdsOfStop);
        result += Vector::byteSize(stopsOfRoute);
        result += Vector::byteSize(tripOfEvent);
        result += sizeof(numTrips);
        result += timeExpandedGraph.byteSize();
        return result;
    }

public:
    std::vector<RAPTOR::Stop> stopData;
    std::vector<RAPTOR::Route> routeData;
    std::vector<RAPTOR::StopEvent> stopEventData;
    std::vector<std::vector<size_t>> stopEventIdsOfStop;
    std::vector<std::vector<StopId>> stopsOfRoute;
    std::vector<TripId> tripOfEvent;
    size_t numTrips;

    // event == transfer node
    // event + numStopEvents == arrival node
    // event + 2 * numStopEvents == departure node
    TimeExpandedGraph timeExpandedGraph;
};
}
