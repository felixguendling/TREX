#include "../../Helpers/IO/Serialization.h"
#include <bitset>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

// Stores the cellIds for a given number of elements. CellIds consist of a _cell
// id per level_, hence Multi Level Partition. The cellid of an element is
// stored inside one integer, where the numberOfCellsPerLevel * least
// significant bits store the cell id for the level 0
class MultiLevelPartition {
private:
    std::vector<uint64_t> ids {};
    uint8_t levels { 1 };
    uint8_t numberOfCellsPerLevel { 1 };

public:
    MultiLevelPartition() = default;
    MultiLevelPartition(uint64_t numberOfNodes, uint8_t levels,
        uint8_t numberOfCellsPerLevel)
        : ids(numberOfNodes, 0)
        , levels(levels)
        , numberOfCellsPerLevel(numberOfCellsPerLevel)
    {
        // no val should be 0
        assert(0 < levels);
        assert(0 < numberOfCellsPerLevel);
        // assert that we have enough space in this uint64_t
        assert(levels * numberOfCellsPerLevel <= 64);
    }

    // simple getter
    inline uint8_t getNumberOfLevels() const { return levels; }
    inline uint8_t getNumberOfCellsPerLevel() const
    {
        return numberOfCellsPerLevel;
    }
    inline uint8_t getNumberOfCellsInLevel(uint8_t level) const
    {
        assert(isLevelValid(level));
        return numberOfCellsPerLevel;
    }

    // Overload the [] operator to access the ID of an element directly
    inline uint64_t operator[](uint64_t node) const
    {
        assert(isNodeValid(node));
        return ids[node];
    }

    // set all ids to 0
    inline void clear() { ids.clear(); }

    // reset, where you can set new vals
    inline void reset(uint64_t newNumberOfNodes, uint8_t newLevels,
        uint8_t newNumberOfCellsPerLevel)
    {
        // no val should be 0
        assert(0 < levels);
        assert(0 < numberOfCellsPerLevel);
        // assert that we have enough space in this uint64_t
        assert(levels * numberOfCellsPerLevel <= 64);

        ids.assign(newNumberOfNodes, 0);
        levels = newLevels;
        numberOfCellsPerLevel = newNumberOfCellsPerLevel;
    }

    // set the correct cellids per level given a node and a global id
    inline void set(uint64_t node, int globalId)
    {
        assert(isNodeValid(node));
        assert(0 <= globalId && globalId < std::pow(numberOfCellsPerLevel, levels));

        for (uint8_t level(0); level < levels; ++level) {
            uint8_t cellId = globalId % numberOfCellsPerLevel;
            setCellIdOfNodeAtLevel(node, cellId, level);
            globalId /= numberOfCellsPerLevel;
        }
    }

    // sets the correct bit for the given node at the given level
    inline void setCellIdOfNodeAtLevel(uint64_t node, uint8_t cellId,
        uint8_t level)
    {
        assert(isNodeValid(node));
        assert(isCellIdValid(cellId));
        assert(isLevelValid(level));

        ids[node] |= (1ULL << (level * numberOfCellsPerLevel + cellId));
    }

    // clears the cell id of the given node at the given level
    inline void clearLevelOfNode(uint64_t node, uint8_t level)
    {
        assert(isNodeValid(node));
        assert(isLevelValid(level));

        ids[node] &= ~(((1ULL << numberOfCellsPerLevel) - 1)
            << (level * numberOfCellsPerLevel));
    }

    // returns the level of the lowest level, on which node1 and node2 differ.
    // Note that iff ids[node1] == ids[node2], then this returns 16
    inline uint8_t getLowestDifferentLevel(uint64_t node1, uint64_t node2) const
    {
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));

        // Compute XOR of IDs for the two nodes
        uint64_t xorResult = ids[node1] ^ ids[node2];

        return __builtin_ctzll(xorResult) / numberOfCellsPerLevel;
    }

    inline int findCrossingLevel(uint64_t node1, uint64_t node2) const
    {
        return std::min(levels, getLowestDifferentLevel(node1, node2));
    }

    inline uint8_t getLowestCommonLevel(uint64_t node1, uint64_t node2) const
    {
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));

        // Compute XOR of IDs for the two nodes
        uint64_t xorResult = ids[node1] ^ ids[node2];

        // std::cout << std::bitset<64>(xorResult) << std::endl;

        return (64 - __builtin_clzll(xorResult)) / numberOfCellsPerLevel;
    }

    inline uint8_t getLowestCommonLevel(uint64_t stop, uint64_t node1,
        uint64_t node2) const
    {
        assert(isNodeValid(stop));
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));

        // Compute XOR of IDs for the two nodes against the stop
        uint64_t xorResult1 = ids[stop] ^ ids[node1];
        uint64_t xorResult2 = ids[stop] ^ ids[node2];

        return (64 - std::max(__builtin_clzll(xorResult1), __builtin_clzll(xorResult2))) / numberOfCellsPerLevel;
    }

    // now, the stop parameter *is* the cellid
    inline uint8_t getLowestCommonLevelExplizit(uint64_t stop, uint64_t node1,
        uint64_t node2) const
    {
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));
        // Compute XOR of IDs for the two nodes against the stop
        uint64_t xorResult1 = stop ^ ids[node1];
        uint64_t xorResult2 = stop ^ ids[node2];

        return 32 - (std::max(__builtin_clzll(xorResult1), __builtin_clzll(xorResult2)) >> 1);
    }

    // returns true iff node1 and node2 are in the same cell on the given level
    // does not mean, that below and / or above this level, the two nodes are in
    // the same cell
    inline bool isInSameCellOnLevel(uint64_t node1, uint64_t node2,
        uint8_t level) const
    {
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));
        assert(isLevelValid(level));

        uint64_t cellMask = ((1ULL << numberOfCellsPerLevel) - 1)
            << (level * numberOfCellsPerLevel);

        return (ids[node1] & cellMask) == (ids[node2] & cellMask);
    }

    // returns true iff node1 and node2 are in the exact same cell
    inline bool inSameCell(uint64_t node1, uint64_t node2) const
    {
        assert(isNodeValid(node1));
        assert(isNodeValid(node2));
        return ids[node1] == ids[node2];
    }

    inline bool inSameCell(uint64_t node, std::vector<int>& levels,
        std::vector<int>& cellIds) const
    {
        assert(isNodeValid(node));
        assert(levels.size() == cellIds.size());

        uint64_t cellMask = 0;

        for (size_t i = 0; i < levels.size(); ++i) {
            assert(isLevelValid(static_cast<uint8_t>(levels[i])));
            assert(isCellIdValid(static_cast<uint8_t>(cellIds[i])));

            cellMask |= 1ULL << (levels[i] * numberOfCellsPerLevel + cellIds[i]);
        }

        return (ids[node] & cellMask) == cellMask;
    }

    inline std::vector<int> verticesInCell(uint8_t level, uint8_t cellId) const
    {
        assert(isLevelValid(level));
        assert(isCellIdValid(cellId));

        std::vector<int> result;

        uint64_t cellMask = 1ULL << (level * numberOfCellsPerLevel + cellId);

        for (uint64_t node = 0; node < ids.size(); ++node) {
            if ((ids[node] & cellMask) != 0) {
                result.push_back(node);
            }
        }

        return result;
    }

    inline std::vector<int> verticesInCell(std::vector<int>& levels,
        std::vector<int>& cellIds) const
    {
        assert(levels.size() == cellIds.size());

        std::vector<int> result;

        uint64_t cellMask = 0;

        for (size_t i = 0; i < levels.size(); ++i) {
            assert(isLevelValid(static_cast<uint8_t>(levels[i])));
            assert(isCellIdValid(static_cast<uint8_t>(cellIds[i])));

            cellMask |= 1ULL << (levels[i] * numberOfCellsPerLevel + cellIds[i]);
        }

        for (uint64_t node = 0; node < ids.size(); ++node) {
            if ((ids[node] & cellMask) == cellMask) {
                result.push_back(node);
            }
        }

        return result;
    }

    // helper
    inline void printCellId(uint64_t node)
    {
        assert(isNodeValid(node));
        std::cout << node << ": " << std::bitset<64>(ids[node]) << std::endl;
    }

    // Returns a vector of cell IDs for the given node
    inline std::vector<uint8_t> getIds(uint64_t node) const
    {
        assert(isNodeValid(node));

        std::vector<uint8_t> cellIDs;

        for (uint8_t level = 0; level < levels; ++level) {
            uint8_t cellId = static_cast<uint8_t>((ids[node] >> (level * numberOfCellsPerLevel)) & ((1ULL << numberOfCellsPerLevel) - 1));
            cellIDs.push_back(cellId);
        }

        return cellIDs;
    }

    // ****************************
    // asserts
    inline bool isLevelValid(uint8_t level) const { return level < levels; }

    inline bool isCellIdValid(uint8_t cellId) const
    {
        return cellId < numberOfCellsPerLevel;
    }

    inline bool isNodeValid(uint64_t node) const { return node < ids.size(); }
    // ****************************

    inline void serialize(IO::Serialization& serialize) const noexcept
    {
        serialize(levels, numberOfCellsPerLevel, ids);
    }

    inline void deserialize(IO::Deserialization& deserialize) noexcept
    {
        deserialize(levels, numberOfCellsPerLevel, ids);
    }
};
