#pragma once

// taken and adapted from here https://github.com/michaelwegner/CRP/blob/master/datastructures/LevelInfo.h

#include "../../Helpers/Assert.h"
#include "../../Helpers/IO/Serialization.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

class LevelInfo {
private:
    std::vector<uint8_t> offset;

public:
    LevelInfo() = default;
    LevelInfo(const std::vector<uint8_t>& offset)
        : offset(offset)
    {
    }

    size_t getCellNumberOnLevel(size_t l, size_t cellNumber) const
    {
        AssertMsg(0 < l && l < offset.size(), "Level out of bounds!");
        return (cellNumber & ~(~0U << offset[l])) >> offset[l - 1];
    }

    // returns numberOfLevels if in same cell
    size_t getHighestDifferingLevel(const size_t c1, const size_t c2) const
    {
        size_t diff = c1 ^ c2;
        if (diff == 0)
            return offset.size();

        for (int l = (int)offset.size() - 1; l > 0; --l) {
            if (diff >> offset[l - 1] > 0)
                return l;
        }
        return offset.size();
    }

    size_t truncateToLevel(size_t cellNumber, size_t l) const
    {
        AssertMsg(0 < l && l <= getLevelCount(), "Level out of bounds");
        return cellNumber >> offset[l - 1];
    }

    size_t getLevelCount() const
    {
        return offset.size() - 1;
    }

    const std::vector<uint8_t>& getOffsets() const
    {
        return offset;
    }

    void serialize(IO::Serialization& serialize) const noexcept
    {
        serialize(offset);
    }

    void deserialize(IO::Deserialization& deserialize) noexcept
    {
        deserialize(offset);
    }
};
