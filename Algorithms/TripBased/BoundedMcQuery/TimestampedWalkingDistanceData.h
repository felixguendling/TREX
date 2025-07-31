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

class TimestampedWalkingDistanceData {
 public:
  TimestampedWalkingDistanceData(const Data& data)
      : data(data),
        labels(data.numberOfStopEvents(), INFTY),
        timestamps(data.numberOfStopEvents(), 0),
        timestamp(0) {}

 public:
  inline void clear() noexcept { timestamp++; }

  inline int operator()(const StopEventId stopEvent) noexcept {
    AssertMsg(stopEvent < labels.size(),
              "StopEvent " << stopEvent << " is out of bounds!");
    return getLabel(stopEvent);
  }

  inline StopEventId getScanEnd(const StopEventId stopEvent,
                                const StopEventId tripEnd,
                                const int walkingDistance) noexcept {
    for (StopEventId event = stopEvent; event < tripEnd; event++) {
      if (getLabel(event) <= walkingDistance) return event;
    }
    return tripEnd;
  }

  inline void update(const StopEventId stopEvent, const StopEventId tripEnd,
                     const StopEventId routeEnd, const StopIndex tripLength,
                     const int walkingDistance) noexcept {
    StopEventId currentStart = stopEvent;
    StopEventId currentEnd = tripEnd;
    for (; currentStart < routeEnd;
         currentStart += tripLength, currentEnd += tripLength) {
      for (StopEventId event = currentStart; event < currentEnd; event++) {
        int& label = getLabel(event);
        if (label <= walkingDistance) break;
        label = walkingDistance;
      }
    }
  }

 private:
  inline int& getLabel(const StopEventId stopEvent) noexcept {
    if (timestamps[stopEvent] != timestamp) {
      labels[stopEvent] = INFTY;
      timestamps[stopEvent] = timestamp;
    }
    return labels[stopEvent];
  }

  const Data& data;

  std::vector<int> labels;
  std::vector<int> timestamps;
  int timestamp;
};

}  // namespace TripBased
