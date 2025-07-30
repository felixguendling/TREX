#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

template <typename key_t = std::uint32_t>
class IndexedSetBranchless {
 public:
  explicit IndexedSetBranchless(std::size_t n)
      : stamps_(n + 1, 0), elems_(n), write_(0), epoch_(1) {}

  bool insert(std::size_t x, bool do_insert = true) {
    assert(x < stamps_.size());
    std::uint16_t prev = stamps_[x];
    std::uint16_t mask = static_cast<std::uint16_t>(do_insert) &
                         static_cast<std::uint16_t>(prev != epoch_);
    stamps_[x] = prev + (epoch_ - prev) * mask;
    elems_[write_] = static_cast<key_t>(x);
    write_ += mask;
    return mask != 0;
  }

  bool contains(std::size_t x) const {
    assert(x < stamps_.size());
    return stamps_[x] == epoch_;
  }

  std::size_t capacity() const { return elems_.size(); }
  std::size_t size() const { return write_; }

  void clear() {
    ++epoch_;
    write_ = 0;
    if (epoch_ == 0) {
      std::fill(stamps_.begin(), stamps_.end(), 0);
      epoch_ = 1;
    }
  }

  using const_iterator = typename std::vector<key_t>::const_iterator;
  const_iterator begin() const { return elems_.begin(); }
  const_iterator end() const { return elems_.begin() + write_; }

 private:
  std::vector<std::uint16_t> stamps_;
  std::vector<key_t> elems_;
  std::size_t write_;
  std::uint16_t epoch_;
};
