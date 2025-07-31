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

namespace TripBased {

class ReachedIndexRounds {
 public:
  ReachedIndexRounds(const Data& data)
      : data(data), defaultLabels(data.numberOfTrips(), -1), currentRound(0) {
    for (const TripId trip : data.trips()) {
      if (data.numberOfStopsInTrip(trip) > 255)
        warning("Trip ", trip, " has ", data.numberOfStopsInTrip(trip),
                " stops!");
      defaultLabels[trip] = data.numberOfStopsInTrip(trip);
    }
  }

 public:
  inline void clear() noexcept {
    labels.resize(1);
    labels[0] = defaultLabels;
    currentRound = 0;
  }

  inline void startNewRound() noexcept {
    currentRound = labels.size();
    labels.emplace_back(labels.back());
  }

  inline void startNewRound(const size_t round) noexcept {
    while (round >= labels.size()) {
      labels.emplace_back(labels.back());
    }
    currentRound = round;
  }

  inline StopIndex operator()(const TripId trip) const noexcept {
    AssertMsg(trip < labels[currentRound].size(),
              "Trip " << trip << " is out of bounds!");
    return StopIndex(labels[currentRound][trip]);
  }

  inline StopIndex operator()(const TripId trip,
                              const size_t round) const noexcept {
    const size_t trueRound = std::min(round, labels.size() - 1);
    AssertMsg(trip < labels[trueRound].size(),
              "Trip " << trip << " is out of bounds!");
    return StopIndex(labels[trueRound][trip]);
  }

  inline bool alreadyReached(const TripId trip,
                             const u_int8_t index) const noexcept {
    return labels[currentRound][trip] <= index;
  }

  inline void update(const TripId trip, const StopIndex index) noexcept {
    AssertMsg(trip < labels[currentRound].size(),
              "Trip " << trip << " is out of bounds!");
    const TripId routeEnd = data.firstTripOfRoute[data.routeOfTrip[trip] + 1];
    for (TripId i = trip; i < routeEnd; i++) {
      if (labels[currentRound][i] <= index) break;
      labels[currentRound][i] = index;
    }
  }

  inline void updateCopyForward(const TripId trip,
                                const StopIndex index) noexcept {
    AssertMsg(trip < labels[currentRound].size(),
              "Trip " << trip << " is out of bounds!");
    const TripId routeEnd = data.firstTripOfRoute[data.routeOfTrip[trip] + 1];
    for (TripId i = trip; i < routeEnd; i++) {
      if (labels[currentRound][i] <= index) break;
      labels[currentRound][i] = index;
      for (size_t round = currentRound + 1; round < labels.size(); round++) {
        if (labels[round][i] <= index) break;
        labels[round][i] = index;
      }
    }
  }

 private:
  const Data& data;

  std::vector<std::vector<u_int8_t>> labels;

  std::vector<u_int8_t> defaultLabels;

  size_t currentRound;
};

}  // namespace TripBased
