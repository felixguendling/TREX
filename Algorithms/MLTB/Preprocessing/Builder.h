#pragma once

#include "../../../DataStructures/MLTB/MLData.h"
#include "../../../Helpers/Console/Progress.h"
#include "../../../Helpers/MultiThreading.h"
#include "../../../Helpers/String/String.h"
#include "../../TripBased/Query/Profiler.h"
#include "TransferSearch.h"
#include <cmath>
#include <vector>

namespace TripBased {

class Builder {
public:
    Builder(MLData& data)
        : data(data)
        , search(data)
    {
        data.createCompactLayoutGraph();
    }

    void process(std::vector<int>& levels, std::vector<int>& ids, const bool verbose = true)
    {
        std::vector<std::pair<TripId, StopIndex>> stopEvents = data.getBorderStopEvents(levels, ids);

        if (verbose) {
            std::cout << "**** { ";
            for (size_t i(0); i < levels.size(); ++i) {
                std::cout << levels[i] << ": " << ids[i] << " ";
            }

            std::cout << "} # Events: " << (int)stopEvents.size() << std::endl;
        }
        for (auto element : stopEvents) {
            search.run(element.first, element.second, levels, ids);
        }
    }

    // obacht, das ist eine recs function
    void computeCellIds(std::vector<std::vector<int>>& result, std::vector<int> level, int depth, int NUM_LEVELS, int NUM_CELLS_PER_LEVELS)
    {
        if (depth == NUM_LEVELS) {
            result.push_back(level);
            return;
        }

        for (int cell(0); cell < NUM_CELLS_PER_LEVELS; ++cell) {
            std::vector<int> copy(level);
            copy[depth] = cell;
            computeCellIds(result, copy, depth + 1, NUM_LEVELS, NUM_CELLS_PER_LEVELS);
        }
    }

    void generateAllLevelCellIds(std::vector<std::vector<int>>& result, int NUM_LEVELS)
    {
        std::vector<int> currentLevel(NUM_LEVELS, 0);
        computeCellIds(result, currentLevel, 0, NUM_LEVELS, data.numberOfCellsPerLevel());
    }

    void printInfo()
    {
        search.getProfiler().printStatisticsAsCSV();
    }

    void customize(const bool verbose = true)
    {
        for (int level(0); level < data.numberOfLevels(); ++level) {
            std::vector<int> levels(data.numberOfLevels() - level, 0);
            for (size_t i(0); i < levels.size(); ++i)
                levels[i] = data.numberOfLevels() - i - 1;

            std::vector<std::vector<int>> result;
            result.reserve(std::pow(data.numberOfCellsPerLevel(), data.numberOfLevels() - level));

            generateAllLevelCellIds(result, data.numberOfLevels() - level);

            if (verbose)
                std::cout << "**** Level: " << level << ", " << result.size() << " cells! ****" << std::endl;

            Progress progress(result.size());

            for (auto& element : result) {
                process(levels, element, verbose);
                ++progress;
            }
            progress.finished();
            if (verbose) {
                std::cout << "Profiler Stats for Level " << level << std::endl;
                printInfo();
            }
            search.getProfiler().reset();
        }
        data.stopEventGraph[LocalLevel].swap(search.getLocalLevels());
    }

private:
    MLData& data;
    TransferSearch<TripBased::AggregateProfiler> search;
};
}
