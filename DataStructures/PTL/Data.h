#pragma once

#include "../TE/Data.h"

#include <algorithm>
#include <vector>

namespace PTL {

class Data {
public:
    Data() {};

    Data(TE::Data& teData)
        : teData(teData)
        , fwdVertices(teData.numberOfTEVertices())
        , bwdVertices(teData.numberOfTEVertices()) {};

    Data(TE::Data& teData, const std::string fileName)
        : teData(teData)
        , fwdVertices(teData.numberOfTEVertices())
        , bwdVertices(teData.numberOfTEVertices())
    {
        readLabels(fileName);
        sortLabels();
    };

    inline static Data FromBinary(const std::string& fileName) noexcept
    {
        Data data;
        data.deserialize(fileName);
        return data;
    }

    inline bool readLabels(const std::string fileName)
    {
        std::cout << "Reading Labels from " << fileName << " ... " << std::endl;
        std::ifstream file;
        file.open(fileName);

        size_t n;
        file >> n;

        if (n != teData.numberOfTEVertices()) {
            std::cout << "Wrong number of entries!" << std::endl;
            return false;
        }

        fwdVertices.resize(n, {});
        bwdVertices.resize(n, {});

        int dummyDist;

        for (Vertex v = Vertex(0); v < n; ++v) {
            size_t s;

            // bwd
            file >> s;
            bwdVertices[v].resize(s);
            for (size_t i = 0; i < s; ++i) {
                file >> bwdVertices[v][i];
                file >> dummyDist;
            }

            // fwd
            file >> s;
            fwdVertices[v].resize(s);
            for (size_t i = 0; i < s; ++i) {
                file >> fwdVertices[v][i];
                file >> dummyDist;
            }
        }
        file >> std::ws;
        file.close();
        std::cout << "Done!" << std::endl;
        return file.eof() && !file.fail();
    }

    inline void clear() noexcept
    {
        AssertMsg(fwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");
        AssertMsg(bwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");

        for (Vertex v = Vertex(0); v < (teData.numberOfTEVertices()); ++v) {
            fwdVertices.clear();
            bwdVertices.clear();
        }
    }

    inline void sortLabels() noexcept
    {
        AssertMsg(fwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");
        AssertMsg(bwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");

        for (Vertex v = Vertex(0); v < (teData.numberOfTEVertices()); ++v) {
            std::sort(fwdVertices[v].begin(), fwdVertices[v].end());
            std::sort(bwdVertices[v].begin(), bwdVertices[v].end());
        }
    }

    inline size_t numberOfStops() const noexcept { return teData.numberOfStops(); }
    inline bool isStop(const StopId stop) const noexcept { return stop < numberOfStops(); }
    inline Range<StopId> stops() const noexcept { return Range<StopId>(StopId(0), StopId(numberOfStops())); }

    inline size_t numberOfTrips() const noexcept { return teData.numTrips; }
    inline bool isTrip(const TripId route) const noexcept { return route < numberOfTrips(); }
    inline Range<TripId> trips() const noexcept { return Range<TripId>(TripId(0), TripId(numberOfTrips())); }

    inline size_t numberOfStopEvents() const noexcept { return teData.events.size(); }

    inline bool isEvent(const Vertex event) const noexcept { return teData.isEvent(event); }
    inline bool isDepartureEvent(const Vertex event) const noexcept { return teData.isDepartureEvent(event); }
    inline bool isArrivalEvent(const Vertex event) const noexcept { return teData.isArrivalEvent(event); }

    inline void printInfo() const noexcept
    {
        size_t minSizeFWD = teData.numberOfTEVertices();
        size_t maxSizeFWD = 0;
        size_t totalSizeFWD = 0;

        size_t minSizeBWD = teData.numberOfTEVertices();
        size_t maxSizeBWD = 0;
        size_t totalSizeBWD = 0;

        AssertMsg(fwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");
        AssertMsg(bwdVertices.size() == (teData.numberOfTEVertices()), "Not the same size!");

        for (Vertex v = Vertex(0); v < teData.numberOfTEVertices(); ++v) {
            minSizeFWD = std::min(minSizeFWD, fwdVertices[v].size());
            maxSizeFWD = std::max(maxSizeFWD, fwdVertices[v].size());

            totalSizeFWD += fwdVertices[v].size();

            minSizeBWD = std::min(minSizeBWD, bwdVertices[v].size());
            maxSizeBWD = std::max(maxSizeBWD, bwdVertices[v].size());

            totalSizeBWD += bwdVertices[v].size();
        }

        std::cout << "PTL public transit data:" << std::endl;
        std::cout << "   Number of Stops:           " << std::setw(12) << String::prettyInt(teData.numberOfStops()) << std::endl;
        std::cout << "   Number of Trips:           " << std::setw(12) << String::prettyInt(teData.numberOfTrips()) << std::endl;
        std::cout << "   Number of TE Vertices:     " << std::setw(12) << String::prettyInt(teData.timeExpandedGraph.numVertices()) << std::endl;
        std::cout << "   Number of TE Edges:        " << std::setw(12) << String::prettyInt(teData.timeExpandedGraph.numEdges()) << std::endl;
        std::cout << "   Forward Labels:" << std::endl;
        std::cout << "      Min # of hubs:          " << std::setw(12) << String::prettyInt(minSizeFWD) << std::endl;
        std::cout << "      Max # of hubs:          " << std::setw(12) << String::prettyInt(maxSizeFWD) << std::endl;
        std::cout << "      Avg # of hubs:          " << std::setw(12) << String::prettyDouble(static_cast<double>(totalSizeFWD) / (teData.numberOfTEVertices())) << std::endl;
        std::cout << "   Backward Labels:" << std::endl;
        std::cout << "      Min # of hubs:          " << std::setw(12) << String::prettyInt(minSizeBWD) << std::endl;
        std::cout << "      Max # of hubs:          " << std::setw(12) << String::prettyInt(maxSizeBWD) << std::endl;
        std::cout << "      Avg # of hubs:          " << std::setw(12) << String::prettyDouble(static_cast<double>(totalSizeBWD) / (teData.numberOfTEVertices())) << std::endl;
        std::cout << "   Total size:                " << std::setw(12) << String::bytesToString(byteSize()) << std::endl;
    }

    inline void serialize(const std::string& fileName) const noexcept
    {
        IO::serialize(fileName, fwdVertices, bwdVertices);
        teData.serialize(fileName + ".te");
    }

    inline void deserialize(const std::string& fileName) noexcept
    {
        IO::deserialize(fileName, fwdVertices, bwdVertices);
        teData.deserialize(fileName + ".te");
    }

    inline long long byteSize() const noexcept
    {
        long long result = Vector::byteSize(fwdVertices);
        result += Vector::byteSize(bwdVertices);
        result += teData.byteSize();
        return result;
    }

    inline std::vector<Vertex>& getFwdHubs(const Vertex vertex) noexcept
    {
        AssertMsg(teData.isEvent(vertex), "Vertex is not valid!");

        return fwdVertices[vertex];
    }

    inline std::vector<Vertex>& getBwdHubs(const Vertex vertex) noexcept
    {
        AssertMsg(teData.isEvent(vertex), "Vertex is not valid!");

        return bwdVertices[vertex];
    }

    TE::Data teData;
    std::vector<std::vector<Vertex>> fwdVertices;
    std::vector<std::vector<Vertex>> bwdVertices;
};
}
