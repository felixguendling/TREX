// NOTE: das habe ich nicht angepasst an "numberOfCellsPerLevel == 2", wird prop
// nicht laufen
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
  Builder(MLData &data)
      : data(data), search(data)
        /* , incommingEventsOfCell(0) */
        ,
        avgPathLengthPerLevel(data.numberOfLevels(), 0) {
    data.createCompactLayoutGraph();
  }

  void process(std::vector<int> &levels, std::vector<int> &ids,
               size_t indexOfCell) {
    std::vector<std::pair<TripId, StopIndex>> stopEvents =
        data.getBorderStopEvents(levels, ids);

    /* incommingEventsOfCell[indexOfCell] = stopEvents.size(); */

    for (auto element : stopEvents) {
      search.run(element.first, element.second, levels, ids);
    }
  }

  // obacht, das ist eine recs function
  void computeCellIds(std::vector<std::vector<int>> &result,
                      std::vector<int> level, int depth, int NUM_LEVELS,
                      int NUM_CELLS_PER_LEVELS) {
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

  void generateAllLevelCellIds(std::vector<std::vector<int>> &result,
                               int NUM_LEVELS) {
    std::vector<int> currentLevel(NUM_LEVELS, 0);
    computeCellIds(result, currentLevel, 0, NUM_LEVELS,
                   data.numberOfCellsPerLevel());
  }

  void printInfo() { search.getProfiler().printStatisticsAsCSV(); }

  void customize(const bool verbose = true) {
    for (int level(0); level < data.numberOfLevels(); ++level) {
      std::vector<int> levels(data.numberOfLevels() - level, 0);
      for (size_t i(0); i < levels.size(); ++i)
        levels[i] = data.numberOfLevels() - i - 1;

      std::vector<std::vector<int>> result;
      result.reserve(std::pow(data.numberOfCellsPerLevel(),
                              data.numberOfLevels() - level));

      generateAllLevelCellIds(result, data.numberOfLevels() - level);

      // STATS
      size_t indexOfCell = 0;
      /* incommingEventsOfCell.assign(std::pow(data.numberOfCellsPerLevel(),
       * data.numberOfLevels() - level), 0); */

      if (verbose)
        std::cout << "**** Level: " << level << ", " << result.size()
                  << " cells! ****" << std::endl;

      Progress progress(result.size());

      for (auto &element : result) {
        process(levels, element, indexOfCell);
        ++progress;
        ++indexOfCell;
      }

      /* search.addCollectShortcuts(); */
      progress.finished();

      if (verbose) {
        std::cout << "##### Stats for Level " << level << std::endl;
        printInfo();

        avgPathLengthPerLevel[level] = search.getAvgPathLengthPerLevel();
        std::cout << "\"Avg. # of Transfers Unpacked\","
                  << avgPathLengthPerLevel[level] << std::endl;
        std::cout << "\"# of added shortcuts\","
                  << search.getNumberOfAddedShortcuts() << std::endl;
        /* std::cout << "Cell Index, Incomming Events\n"; */
        /* for (size_t i(0); i < incommingEventsOfCell.size(); ++i) { */
        /*     std::cout << (int)i << "," << incommingEventsOfCell[i] << "\n";
         */
        /* } */
        std::cout << "###############################" << std::endl;
      }
      search.getProfiler().reset();
      search.resetStats();

      indexOfCell = 0;
    }

    data.stopEventGraph[LocalLevel].swap(search.getLocalLevels());
    /* data.localLevelOfTrip = search.getLocalLevelOfTrips(); */

    if (verbose) {
      Graph::printInfo(search.getAugmentedGraph());
    }

    /* Graph::copy(search.getAugmentedGraph(), data.stopEventGraph); */

    /*         if (verbose) { */
    /*             std::vector<size_t> tripBuckets(data.numberOfLevels()+1, 0);
     */

    /*             for (auto& level : search.getLocalLevelOfTrips()) { */
    /*                 ++tripBuckets[level]; */
    /*             } */

    /*             std::cout << "Trip Distribution over Levels:" << std::endl;
     */
    /*             for (size_t level(0); level < tripBuckets.size(); ++level) */
    /*                 std::cout << level << "\t" << tripBuckets[level] <<
     * std::endl; */
    /*         } */
  }

private:
  MLData &data;
  TransferSearch<TripBased::AggregateProfiler> search;

  // to collect stats
  // vector to count how many incomming events need to be processed at cell [i]
  /* std::vector<uint64_t> incommingEventsOfCell; */
  std::vector<double> avgPathLengthPerLevel;
};
} // namespace TripBased
