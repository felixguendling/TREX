#pragma once

#include <algorithm>
#include <vector>

#include "../Helpers/Assert.h"
#include "../Helpers/IO/Serialization.h"

class UnionFind {

public:
    UnionFind(const int n = 0)
        : parent(n, n)
        , n(n)
    {
    }

    inline void clear() noexcept
    {
        parent.assign(n, n);
    }

    inline int find(const int i) noexcept
    {
        if (parent[i] >= n) {
            return i;
        } else {
            parent[i] = find(parent[i]);
            return parent[i];
        }
    }
    inline int operator()(const int i) noexcept
    {
        return find(i);
    }

    inline void unite(const int i, const int j) noexcept
    {
        if (find(i) != find(j)) {
            link(find(i), find(j));
        }
    }
    inline void operator()(const int i, const int j) noexcept
    {
        unite(i, j);
    }

    inline std::vector<int> getParent() const
    {
        return parent;
    }

    inline void serialize(IO::Serialization& serialize) const noexcept
    {
        serialize(n, parent);
    }

    inline void deserialize(IO::Deserialization& deserialize) noexcept
    {
        deserialize(n, parent);
    }

protected:
    inline void link(const int i, const int j) noexcept
    {
        Assert(parent[i] >= n);
        Assert(parent[j] >= n);
        Assert(i != j);
        if (parent[i] < parent[j]) {
            parent[i] = j;
        } else if (parent[j] < parent[i]) {
            parent[j] = i;
        } else {
            parent[i] = j;
            parent[j]++;
        }
    }

    std::vector<int> parent;
    int n;
};
