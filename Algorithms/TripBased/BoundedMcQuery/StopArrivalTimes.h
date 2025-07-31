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

class StopArrivalTimes {
 public:
  StopArrivalTimes(const Data& data)
      : data(data),
        defaultLabels(data.numberOfStops(), INFTY),
        currentRound(0) {}

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

  inline int operator()(const StopId stop, const size_t round) const noexcept {
    const size_t trueRound = std::min(round, labels.size() - 1);
    return labels[trueRound][stop];
  }

  inline void update(const StopEventId stopEvent) noexcept {
    const StopId stop = data.arrivalEvents[stopEvent].stop;
    const int arrivalTime = data.arrivalEvents[stopEvent].arrivalTime;
    labels[currentRound][stop] =
        std::min(labels[currentRound][stop], arrivalTime);
  }

  inline void updateCopyForward(const StopEventId stopEvent) noexcept {
    const StopId stop = data.arrivalEvents[stopEvent].stop;
    const int arrivalTime = data.arrivalEvents[stopEvent].arrivalTime;
    for (size_t round = currentRound; round < labels.size(); round++) {
      if (labels[round][stop] <= arrivalTime) break;
      labels[round][stop] = arrivalTime;
    }
  }

 private:
  const Data& data;

  std::vector<std::vector<int>> labels;
  std::vector<int> defaultLabels;

  size_t currentRound;
};

}  // namespace TripBased
