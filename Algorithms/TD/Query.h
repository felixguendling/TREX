#pragma once

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "../../DataStructures/Attributes/AttributeNames.h"
#include "../../DataStructures/Container/ExternalKHeap.h"
#include "../../DataStructures/Container/Set.h"
#include "../../Helpers/Meta.h"
#include "../../Helpers/String/String.h"
#include "../../Helpers/Timer.h"
#include "../../Helpers/Types.h"
#include "../../Helpers/Vector/Vector.h"

namespace TD {

template <typename GRAPH, bool DEBUG = false>
class TDDijkstra {
public:
    using Graph = GRAPH;
    static constexpr bool Debug = DEBUG;
    using Type = TDDijkstra<Graph, Debug>;

public:
    struct VertexLabel : public ExternalKHeapElement {
        VertexLabel()
            : ExternalKHeapElement()
            , arrivalTime(intMax)
            , parent(noVertex)
            , timeStamp(-1)
        {
        }
        inline void reset(int time)
        {
            arrivalTime = intMax;
            parent = noVertex;
            timeStamp = time;
        }
        inline bool hasSmallerKey(const VertexLabel* other) const
        {
            return arrivalTime < other->arrivalTime;
        }

        int arrivalTime;
        Vertex parent;
        int timeStamp;
    };

public:
    TDDijkstra(const GRAPH& graph, const std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& times)
        : graph(graph)
        , times(times)
        , Q(graph.numVertices())
        , label(graph.numVertices())
        , timeStamp(0)
        , settleCount(0)
    {
    }

    TDDijkstra(const GRAPH&&, const std::vector<int>&) = delete;
    TDDijkstra(const GRAPH&, const std::vector<int>&&) = delete;
    TDDijkstra(const GRAPH&&) = delete;

    template <AttributeNameType ATTRIBUTE_NAME>
    TDDijkstra(const GRAPH&&, const AttributeNameWrapper<ATTRIBUTE_NAME>) = delete;

    template <typename SETTLE = NO_OPERATION, typename STOP = NO_OPERATION, typename PRUNE_EDGE = NO_OPERATION>
    inline void run(const Vertex source, const int departureTime = 0, const Vertex target = noVertex, const SETTLE& settle = NoOperation,
        const STOP& stop = NoOperation, const PRUNE_EDGE& pruneEdge = NoOperation) noexcept
    {
        clear();
        addSource(source, departureTime);
        run(target, settle, stop, pruneEdge);
    }

    template <typename SETTLE = NO_OPERATION, typename STOP = NO_OPERATION, typename PRUNE_EDGE = NO_OPERATION>
    inline void run(const Vertex source, IndexedSet<false, Vertex>& targets, const SETTLE& settle = NoOperation,
        const STOP& stop = NoOperation, const PRUNE_EDGE& pruneEdge = NoOperation) noexcept
    {
        clear();
        addSource(source);
        run(
            noVertex,
            [&](const Vertex u) {
                settle(u);
                targets.remove(u);
            },
            [&]() { return stop() || targets.empty(); }, pruneEdge);
    }

    template <typename SOURCE_CONTAINER, typename SETTLE = NO_OPERATION, typename STOP = NO_OPERATION,
        typename PRUNE_EDGE = NO_OPERATION, typename = decltype(std::declval<SOURCE_CONTAINER>().begin())>
    inline void run(const SOURCE_CONTAINER& sources, const Vertex target = noVertex, const SETTLE& settle = NoOperation,
        const STOP& stop = NoOperation, const PRUNE_EDGE& pruneEdge = NoOperation) noexcept
    {
        clear();
        for (const Vertex source : sources) {
            addSource(source);
        }
        run(target, settle, stop, pruneEdge);
    }

    inline void clear() noexcept
    {
        if constexpr (Debug) {
            timer.restart();
            settleCount = 0;
        }
        Q.clear();
        timeStamp++;
    }

    inline void addSource(const Vertex source, const int arrivalTime = 0) noexcept
    {
        VertexLabel& sourceLabel = getLabel(source);
        sourceLabel.arrivalTime = arrivalTime;
        Q.update(&sourceLabel);
    }

    inline void run() noexcept
    {
        run(noVertex, NoOperation, NoOperation, NoOperation);
    }

    template <typename SETTLE, typename STOP = NO_OPERATION, typename PRUNE_EDGE = NO_OPERATION,
        typename = decltype(std::declval<SETTLE>()(std::declval<Vertex>()))>
    inline void run(const Vertex target, const SETTLE& settle, const STOP& stop = NoOperation,
        const PRUNE_EDGE& pruneEdge = NoOperation) noexcept
    {
        while (!Q.empty()) {
            if (stop())
                break;
            VertexLabel* uLabel = Q.extractFront();
            const Vertex u = Vertex(uLabel - &(label[0]));
            if (u == target)
                break;
            for (const Edge edge : graph.edgesFrom(u)) {
                const Vertex v = graph.get(ToVertex, edge);
                VertexLabel& vLabel = getLabel(v);
                if (pruneEdge(u, edge))
                    continue;
                // duration != -1 => footpath
                int arrivalTime = uLabel->arrivalTime;

                if (graph.get(TravelTime, edge) != -1) {
                    arrivalTime += graph.get(TravelTime, edge);
                } else {
                    assert(graph.get(Index, edge) < times.size());
                    arrivalTime += evaluateEdge(graph.get(Index, edge), arrivalTime);
                }
                if (vLabel.arrivalTime > arrivalTime) {
                    vLabel.arrivalTime = arrivalTime;
                    vLabel.parent = u;
                    Q.update(&vLabel);
                }
            }
            settle(u);
            if constexpr (Debug)
                settleCount++;
        }
        if constexpr (Debug) {
            std::cout << "Settled Vertices = " << String::prettyInt(settleCount) << std::endl;
            std::cout << "Time = " << String::msToString(timer.elapsedMilliseconds()) << std::endl;
        }
    }

    inline bool reachable(const Vertex vertex) const noexcept
    {
        return label[vertex].timeStamp == timeStamp;
    }

    inline bool visited(const Vertex vertex) const noexcept
    {
        return label[vertex].timeStamp == timeStamp;
    }

    inline int getDistance(const Vertex vertex) const noexcept
    {
        if (visited(vertex))
            return label[vertex].arrivalTime;
        return -1;
    }

    inline Vertex getParent(const Vertex vertex) const noexcept
    {
        if (visited(vertex))
            return label[vertex].parent;
        return noVertex;
    }

    inline std::set<Vertex> getChildren(const Vertex vertex) const noexcept
    {
        if (visited(vertex)) {
            std::set<Vertex> children;
            for (Vertex child : graph.outgoingNeighbors(vertex)) {
                if (label[child].parent == vertex) {
                    children.insert(child);
                }
            }
            return children;
        }
        return std::set<Vertex>();
    }

    inline Vertex getQFront() const noexcept
    {
        if (Q.empty())
            return noVertex;
        return Vertex(Q.front() - &(label[0]));
    }

    inline std::vector<Vertex> getReversePath(const Vertex to) const noexcept
    {
        std::vector<Vertex> path;
        if (!visited(to))
            return path;
        path.push_back(to);
        while (label[path.back()].parent != noVertex) {
            path.push_back(label[path.back()].parent);
        }
        return path;
    }

    inline std::vector<Vertex> getPath(const Vertex to) const noexcept
    {
        return Vector::reverse(getReversePath(to));
    }

    inline int getSettleCount() const noexcept
    {
        return settleCount;
    }

private:
    inline VertexLabel& getLabel(const Vertex vertex) noexcept
    {
        VertexLabel& result = label[vertex];
        if (result.timeStamp != timeStamp)
            result.reset(timeStamp);
        return result;
    }

    int evaluateEdge(const size_t index, const int arrivalTime) const
    {
        AssertMsg(index < times.size(), "Index " << index << " is out of bounds (" << times.size() << ")!");

        size_t i = 0;

        while (times[index][i].first < static_cast<uint32_t>(arrivalTime))
            ++i;

        return times[index][i].second;
    }

private:
    const GRAPH& graph;
    const std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& times;

    ExternalKHeap<2, VertexLabel> Q;

    std::vector<VertexLabel> label;
    int timeStamp;

    int settleCount;
    Timer timer;
};
}
