#pragma once

#include "../../Helpers/Assert.h"
#include "../../Helpers/Helpers.h"
#include "../../Helpers/IO/Serialization.h"
#include <vector>

namespace TripBased {

// a PackedTransfer (i.e., a uint32_t) is split as follows:
// the next 24 bits are TripID (global TripID)
// the last 8 bits are Position

using TransferVector = std::vector<PackedTransfer>;

class DynamicEventGraph {
public:
    DynamicEventGraph() = default;

    // just clear everything
    inline void clear() noexcept
    {
        toAdjs.clear();
        transfers.clear();
        numEdges = 0;
    }

    inline std::vector<std::vector<size_t>>& getToAdjs() noexcept
    {
        return toAdjs;
    }

    inline std::vector<TransferVector>& getTransfers() noexcept
    {
        return transfers;
    }

    inline size_t& getNumEdges() noexcept { return numEdges; }

    inline size_t numberOfEdges() const noexcept { return numEdges; }

    inline bool isVertex(TripId trip, StopIndex position) const noexcept
    {
        return trip < toAdjs.size() && position < toAdjs[trip].size() - 1;
    }

    inline bool isEdge(TripId trip, Edge edge) const noexcept
    {
        return trip < toAdjs.size() && edge < transfers[trip].size();
    }

    inline size_t beginEdgeFrom(const TripId trip,
        const StopIndex position) const
    {
        // note: position can be valid, but also +1
        AssertMsg(isVertex(trip, position) || position == toAdjs[trip].size() - 1,
            "Trip and / or position is not valid");

        return toAdjs[trip][position];
    }

    inline auto& getToAdjsOfTrip(const TripId trip)
    {
        AssertMsg(trip < transfers.size(), "Trip is not valid");

        return toAdjs[trip];
    }

    inline auto& getTransfersOfTrip(const TripId trip)
    {
        AssertMsg(trip < transfers.size(), "Trip is not valid");

        return transfers[trip];
    }

    inline PackedTransfer getTransfer(TripId trip, Edge transferId) const
    {
        AssertMsg(isEdge(trip, transferId), "Trip and or transferId is not valid");
        return transfers[trip][transferId];
    }

    inline void serialize(const std::string& fileName) const noexcept
    {
        IO::serialize(fileName, toAdjs, transfers, numEdges);
    }

    inline void deserialize(const std::string& fileName) noexcept
    {
        IO::deserialize(fileName, toAdjs, transfers, numEdges);
    }

private:
    // maps *per Trip* an event to its start position in the transfers /
    // travelTimes vector
    std::vector<std::vector<size_t>> toAdjs;

    // contains *per Trip* this "packed" transfer
    std::vector<TransferVector> transfers;

    // holds the total number of edges
    size_t numEdges { 0 };
};

} // namespace TripBased
