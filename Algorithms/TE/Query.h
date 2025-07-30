#pragma once

#include <limits>
#include <queue>
#include <set>
#include <vector>

#include "../../DataStructures/Container/radix_heap.h"
#include "../../DataStructures/TE/Data.h"
#include "Profiler.h"

namespace TE {

template <typename PROFILER = NoProfiler, bool NODE_BLOCKING = false>
class Query {
 public:
  using Profiler = PROFILER;

  Query(const Data &data)
      : data(data),
        weight(data.timeExpandedGraph[TravelTime]),
        label(data.timeExpandedGraph.numVertices()),
        Q(),
        timeStamp(0) {
    profiler.registerPhases({PHASE_CLEAR, PHASE_FIND_FIRST_VERTEX, PHASE_RUN});
    profiler.registerMetrics({METRIC_SEETLED_VERTICES, METRIC_RELAXED_EDGES,
                              METRIC_FOUND_SOLUTIONS,
                              METRIC_POPPED_BUT_IGNORED});
  }

  int run(const StopId source, const int departureTime,
          const StopId target) noexcept {
    profiler.start();

    AssertMsg(data.isStop(source), "Source is not valid!");
    AssertMsg(data.isStop(target), "Target is not valid!");
    AssertMsg(0 <= departureTime, "Time is negative!");

    profiler.startPhase();
    Vertex firstReachableNode = Vertex(
        data.getFirstReachableDepartureVertexAtStop(source, departureTime));
    if (firstReachableNode == data.numberOfTEVertices()) [[unlikely]]
      return -1;
    AssertMsg(data.isDepartureEvent(firstReachableNode),
              "Invalid departure vertex!");
    profiler.donePhase(PHASE_FIND_FIRST_VERTEX);

    profiler.startPhase();
    clear();
    profiler.donePhase(PHASE_CLEAR);

    profiler.startPhase();
    addSource(firstReachableNode, 0);

    auto targetPruning = [&]() {
      Vertex front = getQFront();
      return (data.timeExpandedGraph.get(StopVertex, front) == target);
    };

    auto settle = [&](const Vertex /*v*/) {
      profiler.countMetric(METRIC_SEETLED_VERTICES);
    };

    auto pruneEdge = [&](const Vertex /*from*/, const Edge /*e*/) {
      profiler.countMetric(METRIC_RELAXED_EDGES);
      return false;
    };

    run(settle, targetPruning, pruneEdge);
    profiler.donePhase(PHASE_RUN);

    Vertex finalVertex = getQFront();
    AssertMsg(finalVertex == noVertex ||
                  data.timeExpandedGraph.get(StopVertex, finalVertex) == target,
              "last vertex was neither noVertex or at the target?");

    if (finalVertex != noVertex) {
      profiler.countMetric(METRIC_FOUND_SOLUTIONS);
    }

    profiler.done();
    return (finalVertex == noVertex ? -1 : getDistance(finalVertex));
  }

  inline const Profiler &getProfiler() const noexcept { return profiler; }

 private:
  struct VertexLabel {
    std::uint32_t distance = -1;
    Vertex parent = noVertex;
    std::uint32_t timeStamp = -1;

    void reset(std::uint32_t time) {
      distance = -1;
      parent = noVertex;
      timeStamp = time;
    }
  };

  void clear() noexcept {
    Q.clear();
    timeStamp++;
  }

  void addSource(Vertex source, std::uint32_t distance = 0) noexcept {
    VertexLabel &sourceLabel = getLabel(source);
    sourceLabel.distance = distance;
    Q.push(static_cast<uint32_t>(source), distance);
  }

  VertexLabel &getLabel(Vertex v) noexcept {
    VertexLabel &lbl = label[v];
    if (lbl.timeStamp != timeStamp) {
      lbl.reset(timeStamp);
    }
    return lbl;
  }

  bool visited(Vertex v) const noexcept {
    return label[v].timeStamp == timeStamp;
  }

  std::uint32_t getDistance(Vertex v) const noexcept {
    return visited(v) ? label[v].distance : -1;
  }

  Vertex getQFront() noexcept {
    return Q.empty() ? noVertex : Vertex(Q.top_key());
  }

  template <typename SETTLE, typename STOP, typename PRUNE_EDGE>
  void run(const SETTLE &settle, const STOP &stop,
           const PRUNE_EDGE &pruneEdge) noexcept {
    while (!Q.empty()) {
      if (stop()) break;

      auto [uKey, dist] = Q.topAndPop();
      Vertex u(uKey);

      VertexLabel &uLabel = getLabel(u);
      if (dist != uLabel.distance) [[unlikely]] {
        profiler.countMetric(METRIC_POPPED_BUT_IGNORED);
        continue;
      }

      for (Edge e : data.timeExpandedGraph.edgesFrom(u)) {
        Vertex v = data.timeExpandedGraph.get(ToVertex, e);
        if (pruneEdge(u, e)) continue;

        VertexLabel &vLabel = getLabel(v);
        std::uint32_t alt = uLabel.distance + weight[e];
        if (alt < vLabel.distance) {
          vLabel.distance = alt;
          vLabel.parent = u;
          Q.push(static_cast<uint32_t>(v), alt);
        }
      }

      settle(u);
    }
  }

  const Data &data;
  std::vector<int> weight;
  std::vector<VertexLabel> label;
  radix_heap::pair_radix_heap<uint32_t, uint32_t> Q;
  std::uint32_t timeStamp;
  Profiler profiler;
};

}  // namespace TE
