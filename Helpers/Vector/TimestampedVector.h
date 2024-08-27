#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <type_traits>
#include <algorithm>

template <typename TYPE, typename TYPEOFINT = uint32_t, typename Allocator = std::allocator<TYPE>>
class TimestampedVector
{
public:
    TimestampedVector(size_t capacity, TYPE invalidType)
        : values(capacity, invalidType), 
          timestamps(capacity, 0), 
          timeNow(1), 
          invalidType(invalidType) {
    }

    TimestampedVector(const TimestampedVector&) = default;
    TimestampedVector(TimestampedVector&&) noexcept = default;

    TimestampedVector& operator=(const TimestampedVector&) = default;
    TimestampedVector& operator=(TimestampedVector&&) noexcept = default;

    void set(size_t index, TYPE type) noexcept {
        assert(isIndexValid(index));

        values[index] = type;
        timestamps[index] = timeNow;
    }

    // Method to get the element at a specific index
    TYPE get(size_t index) const noexcept {
        assert(isIndexValid(index));
        if (timestamps[index] == timeNow) {
            return values[index];
        }
        return invalidType;
    }

    // Range-checked access with exception handling
    TYPE at(size_t index) const {
        assert(isIndexValid(index));
        return get(index);
    }

    // Clear all elements and increment the timestamp
    void clear() noexcept {
        ++timeNow;
        if (timeNow == 0) {
            std::fill(values.begin(), values.end(), invalidType);
            std::fill(timestamps.begin(), timestamps.end(), 0);
            ++timeNow;
        }
    }

    // Reserve capacity
    void reserve(size_t newCapacity) {
        values.reserve(newCapacity);
        timestamps.reserve(newCapacity);
    }

    // Get current capacity
    size_t capacity() const noexcept {
        return values.capacity();
    }

    // Method to print all elements for debugging
    void print() const {
        for (size_t i = 0; i < values.size(); ++i) {
            std::cout << "Index " << i << ": Value = " << values[i] 
                      << ", Timestamp = " << timestamps[i] << std::endl;
        }
    }

private:
    inline bool isIndexValid(const size_t index) const noexcept {
        return index < values.size();
    }

    std::vector<TYPE, Allocator> values;
    std::vector<TYPEOFINT> timestamps;

    TYPEOFINT timeNow;
    TYPE invalidType;
};

