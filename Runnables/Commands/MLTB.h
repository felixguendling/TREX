#pragma once

#include <iostream>
#include <string>

#include "../../Algorithms/MLTB/Preprocessing/Builder.h"
#include "../../Algorithms/TripBased/Preprocessing/StopEventGraphBuilder.h"
#include "../../Algorithms/TripBased/Preprocessing/ULTRABuilderTransitive.h"
#include "../../DataStructures/Graph/Graph.h"
#include "../../DataStructures/MLTB/MLData.h"
#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/TripBased/Data.h"

#include "../../DataStructures/Queries/Queries.h"
#include "../../Algorithms/MLTB/Query/MLQueryBitset.h"

#include "../../Helpers/Console/Progress.h"
#include "../../Helpers/MultiThreading.h"
#include "../../Helpers/String/String.h"
#include "../../Shell/Shell.h"

using namespace Shell;

class ApplyPartitionFile : public ParameterizedCommand {
public:
    ApplyPartitionFile(BasicShell& shell)
        : ParameterizedCommand(shell, "applyPartitionFile", "Applies the given partition to the MLTB data. Also give the number of levels and the number of cells per level!")
    {
        addParameter("Input file (Partition File)");
        addParameter("Input file (MLTB Data)");
        addParameter("Show Cut Information?");
    }

    virtual void execute() noexcept
    {
        const std::string raptorFile = getParameter("Input file (MLTB Data)");
        const std::string partitionFile = getParameter("Input file (Partition File)");
        const bool showCut = getParameter<bool>("Show Cut Information?");

        TripBased::MLData data(raptorFile);
        data.printInfo();

        data.createCompactLayoutGraph();
        data.readPartitionFile(partitionFile);
        data.serialize(raptorFile);

        if (showCut)
            data.showCuts();
    }
};

class RAPTORToMLTB : public ParameterizedCommand {
public:
    RAPTORToMLTB(BasicShell& shell)
        : ParameterizedCommand(shell, "raptorToMLTB", "Reads RAPTOR Data, Number of Levels and Number of Cells per Level and saves it to a MLTB Data")
    {
        addParameter("Input file (RAPTOR Data)");
        addParameter("Output file (MLTB Data)");
        addParameter("Number of levels");
        addParameter("Number of cells per level");
        addParameter("Route-based pruning?", "true");
        addParameter("Number of threads", "max");
        addParameter("Pin multiplier", "1");
    }

    virtual void execute() noexcept
    {
        const std::string raptorFile = getParameter("Input file (RAPTOR Data)");
        const std::string mltbFile = getParameter("Output file (MLTB Data)");
        const int numLevels = getParameter<int>("Number of levels");
        const int numCellsPerLevel = getParameter<int>("Number of cells per level");
        const bool routeBasedPruning = getParameter<bool>("Route-based pruning?");
        const int numberOfThreads = getNumberOfThreads();
        const int pinMultiplier = getParameter<int>("Pin multiplier");

        RAPTOR::Data raptor(raptorFile);
        TripBased::MLData data(raptor, numLevels, numCellsPerLevel);

        if (numberOfThreads == 0) {
            if (routeBasedPruning) {
                TripBased::ComputeStopEventGraphRouteBased(data);
            } else {
                TripBased::ComputeStopEventGraph(data);
            }
        } else {
            if (routeBasedPruning) {
                TripBased::ComputeStopEventGraphRouteBased(data, numberOfThreads, pinMultiplier);
            } else {
                TripBased::ComputeStopEventGraph(data, numberOfThreads, pinMultiplier);
            }
        }
        data.addFlagsToStopEventGraph();
        data.printInfo();
        data.serialize(mltbFile);
    }

private:
    inline int getNumberOfThreads() const noexcept
    {
        if (getParameter("Number of threads") == "max") {
            return numberOfCores();
        } else {
            return getParameter<int>("Number of threads");
        }
    }
};

class CreateCompactLayoutGraph : public ParameterizedCommand {
public:
    CreateCompactLayoutGraph(BasicShell& shell)
        : ParameterizedCommand(shell, "createCompactLayoutGraph", "Creates the compact layout graph of the given MLTB data, it writes the Compact Layout Graph into METIS Format. If wanted, write a GRAPHML file.")
    {
        addParameter("Input file (MLTB Data)");
        addParameter("Output file (METIS File)");
        addParameter("Write GRAPHML?");
    }

    virtual void execute() noexcept
    {
        const std::string mltbFile = getParameter("Input file (MLTB Data)");
        const std::string metisFile = getParameter("Output file (METIS File)");
        const bool writeGRAPHML = getParameter<bool>("Write GRAPHML?");

        TripBased::MLData data(mltbFile);
        data.printInfo();

        data.createCompactLayoutGraph();
        data.writeLayoutGraphToMETIS(metisFile, writeGRAPHML);
        data.serialize(mltbFile);
    }
};

class Customization : public ParameterizedCommand {
public:
    Customization(BasicShell& shell)
        : ParameterizedCommand(shell, "customize", "Computes the customization of MLTB")
    {
        addParameter("Input file (MLTB Data)");
        addParameter("Output file (MLTB Data)");
    }

    virtual void execute() noexcept
    {
        const std::string mltbFile = getParameter("Input file (MLTB Data)");
        const std::string output = getParameter("Output file (MLTB Data)");

        TripBased::MLData data(mltbFile);
        data.printInfo();

        TripBased::Builder bobTheBuilder(data);

        bobTheBuilder.customize();

        data.serialize(output);
    }
};

class ShowInfoOfMLTB : public ParameterizedCommand {
public:
    ShowInfoOfMLTB(BasicShell& shell) : ParameterizedCommand(shell, "showInfoOfMLTB", "Shows Information about the given MLTB file.") {
        addParameter("Input file (MLTB Data)");
    }

    virtual void execute() noexcept {
        const std::string tripFile = getParameter("Input file (MLTB Data)");
        TripBased::MLData data(tripFile);
        data.printInfo();

        std::vector<size_t> numGlobalTransfers(data.numberOfLevels(), 0);

        std::vector<std::pair<StopEventId, Edge>> examplesOfTopMostTransfers;

        for (const auto [edge, from] : data.stopEventGraph.edgesWithFromVertex()) {
            std::vector<bool>& flags = data.stopEventGraph.get(ARCFlag, edge);

            for (int level(data.numberOfLevels() - 1); level >= 0; --level) {
                if (flags[level]) {
                    ++numGlobalTransfers[level];

                    if (level == data.numberOfLevels() - 1 && examplesOfTopMostTransfers.size() < 10)
                        examplesOfTopMostTransfers.push_back(std::make_pair(StopEventId(from), edge));
                    break;
                }
            }
        }

        std::cout << "** Number of Global Transfers **" << std::endl;
        std::cout << "Note: Each transfer is counted for it's highest level!"<< std::endl;
        
        for (size_t level(0); level < numGlobalTransfers.size(); ++level) {
            std::cout << "Level " << level << ":       " << String::prettyInt(numGlobalTransfers[level]) << std::endl;
        }

        std::cout << "Here are " << String::prettyInt(examplesOfTopMostTransfers.size()) << " topmost global transfers:" << std::endl;

        for (auto& pair : examplesOfTopMostTransfers) {
            std::cout << "\tFrom Stop " << data.raptorData.stopData[data.getStopOfStopEvent(pair.first)] << " to Stop " << data.raptorData.stopData[data.getStopOfStopEvent(StopEventId(data.stopEventGraph.get(ToVertex, pair.second)))] << std::endl;
            /* std::cout << "\tFrom Trip " << data.raptorData.tripData[data.tripOfStopEvent[pair.first]] << " to Trip " << data.raptorData.tripData[data.tripOfStopEvent[StopEventId(data.stopEventGraph.get(ToVertex, pair.second))]] << std::endl; */
            std::cout << "\tFrom Route " << data.raptorData.routeData[data.routeOfTrip[data.tripOfStopEvent[pair.first]]] << " to Route " << data.raptorData.routeData[data.routeOfTrip[data.tripOfStopEvent[StopEventId(data.stopEventGraph.get(ToVertex, pair.second))]]] << std::endl;
        }
    }
};

class RunMLQuery : public ParameterizedCommand {
public:
    RunMLQuery(BasicShell& shell)
        : ParameterizedCommand(shell, "runMLTBQueries",
            "Runs the given number of random MultiLevel TB queries.")
    {
        addParameter("Input file (MLTB Data)");
        addParameter("Number of queries");
    }

    virtual void execute() noexcept
    {
        const std::string tripFile = getParameter("Input file (MLTB Data)");
        TripBased::MLData data(tripFile);
        data.fixFlags();
        data.printInfo();
        TripBased::MLQueryBitset<TripBased::AggregateProfiler> algorithm(data);

        const size_t n = getParameter<size_t>("Number of queries");
        const std::vector<StopQuery> queries = generateRandomStopQueries(data.numberOfStops(), n);

        size_t numberOfJourneys = 0;

        for (const StopQuery& query : queries) {
            algorithm.run(query.source, query.departureTime, query.target);
            numberOfJourneys += algorithm.getJourneys().size();
        }
        algorithm.getProfiler().printStatistics();
        std::cout << "Avg. Journeys: " << String::prettyDouble(numberOfJourneys / (float) queries.size()) << std::endl;
    }
};

