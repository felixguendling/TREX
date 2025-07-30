#pragma once

#include <queue>
#include <vector>

#include "TBTEGraph.h"

namespace TripBased {

class TopoBFS {
 public:
  using PQueue =
      std::priority_queue<Vertex, std::vector<Vertex>, std::greater<>>;
  TBTEGraph &tbte;
  PQueue Q;

  TopoBFS(TBTEGraph &tbte) : tbte(tbte) {}

  void reset() { Q = PQueue(); }

  void run(std::vector<Vertex> &sources) {
    for (auto s : sources) {
      Q.push(s);
      tbte.distances[s].fill(0);
      tbte.parent[s].fill(static_cast<uint16_t>(-1));
    }
  }
};
}  // namespace TripBased
