#pragma once

#include "../../Algorithms/MLTB/Preprocessing/Builder.h"
#include "../../Algorithms/TripBased/Preprocessing/StopEventGraphBuilder.h"
#include "../../Algorithms/TripBased/Preprocessing/ULTRABuilderTransitive.h"
#include "../../DataStructures/Graph/Graph.h"
#include "../../DataStructures/Graph/Utils/IO.h"
#include "../../DataStructures/MLTB/MLData.h"
#include "../../DataStructures/Queries/Queries.h"
#include "../../DataStructures/RAPTOR/Data.h"
#include "../../DataStructures/TripBased/Data.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "../../Algorithms/MLTB/Query/MLQuery.h"
#include "../../Algorithms/TripBased/Query/TransitiveQuery.h"

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
        data.addInformationToStopEventGraph();
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
        addParameter("Write GRAPHML?", "false");
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
        addParameter("Verbose?", "true");
    }

    virtual void execute() noexcept
    {
        const std::string mltbFile = getParameter("Input file (MLTB Data)");
        const std::string output = getParameter("Output file (MLTB Data)");
        const bool verbose = getParameter<bool>("Verbose?");

        TripBased::MLData data(mltbFile);
        data.printInfo();

        TripBased::Builder bobTheBuilder(data);

        bobTheBuilder.customize(verbose);
        data.serialize(output);
    }
};

class ShowInfoOfMLTB : public ParameterizedCommand {
public:
    ShowInfoOfMLTB(BasicShell& shell)
        : ParameterizedCommand(shell, "showInfoOfMLTB", "Shows Information about the given MLTB file.")
    {
        addParameter("Input file (MLTB Data)");
        addParameter("Write to csv?", "false");
        addParameter("Output file (csv)", "levelDegree");
    }

    virtual void execute() noexcept
    {
        const std::string tripFile = getParameter("Input file (MLTB Data)");
        const bool writeToCSV = getParameter<bool>("Write to csv?");
        const std::string fileName = getParameter("Output file (csv)");
        TripBased::MLData data(tripFile);
        data.printInfo();

        std::vector<size_t> numLocalTransfers(data.numberOfLevels() + 1, 0);
        std::vector<size_t> numLocalEvents(data.numberOfLevels() + 1, 0);
        std::vector<size_t> numHopsPerLevel(data.numberOfLevels() + 1, 0);

        std::vector<std::vector<size_t>> degreePerLevel(data.numberOfLevels() + 1);
        for (size_t l(0); l < (size_t)data.numberOfLevels() + 1; ++l) {
            degreePerLevel[l].assign(data.stopEventGraph.numVertices(), 0);
        }

        for (const auto [edge, from] : data.stopEventGraph.edgesWithFromVertex()) {
            ++degreePerLevel[data.stopEventGraph.get(LocalLevel, edge)][from];
            ++numLocalTransfers[data.stopEventGraph.get(LocalLevel, edge)];
            numHopsPerLevel[data.stopEventGraph.get(LocalLevel, edge)] += data.stopEventGraph.get(Hop, edge);

            /* for (int level(data.numberOfLevels() - 1); level >= 0; --level) { */
            /*     if (data.stopEventGraph.get(LocalLevel, edge) >= level) { */
            /*         ++numLocalTransfers[level]; */
            /*         numHopsPerLevel[level] += data.stopEventGraph.get(Hop, edge); */
            /*         break; */
            /*     } */
            /* } */
        }

        std::cout << "** Number of Local Transfers **" << std::endl;

        for (size_t level(0); level < numLocalTransfers.size(); ++level) {
            std::cout << "Level " << level << ":       " << String::prettyInt(numLocalTransfers[level]) << "    " << String::prettyDouble((100.0 * numLocalTransfers[level] / data.stopEventGraph.numEdges())) << " %" << std::endl;
        }

        std::cout << "** Avg # of hops per Level **" << std::endl;

        for (size_t level(0); level < numLocalTransfers.size(); ++level) {
            std::cout << "Level " << level << ":      " << String::prettyDouble(numHopsPerLevel[level] / (double)numLocalTransfers[level]) << std::endl;
        }

        /* for (StopEventId event(0); event < data.numberOfStopEvents(); ++event) { */
        /*     ++numLocalEvents[data.getLocalLevelOfEvent(event)]; */
        /* } */

        /* std::cout << "** Number of local Events **" << std::endl; */
        /* for (size_t level(0); level < numLocalTransfers.size(); ++level) { */
        /*     std::cout << "Level " << level << ":       " << String::prettyInt(numLocalEvents[level]) << "    " << String::prettyDouble((100.0 * numLocalEvents[level] / data.numberOfStopEvents())) << " %" << std::endl; */
        /* } */

        if (writeToCSV) {
            for (size_t l(0); l < (size_t)data.numberOfLevels() + 1; ++l) {
                std::ofstream outputfile(fileName + ".level" + std::to_string(l) + ".csv");

                outputfile << "StopEventId,Degree\n";
                for (size_t i(0); i < degreePerLevel[l].size(); ++i)
                    outputfile << i << "," << degreePerLevel[l][i] << "\n";
            }
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
        addParameter("Compare to TB?");
        addParameter("TB Input for eval");
    }

    virtual void execute() noexcept
    {
        const std::string tripFile = getParameter("Input file (MLTB Data)");
        const bool eval = getParameter<bool>("Compare to TB?");
        const std::string evalFile = getParameter("TB Input for eval");

        TripBased::MLData data(tripFile);
        data.printInfo();
        TripBased::MLQuery<TripBased::AggregateProfiler> algorithm(data);

        const size_t n = getParameter<size_t>("Number of queries");
        const std::vector<StopQuery> queries = generateRandomStopQueries(data.numberOfStops(), n);

        std::vector<std::vector<std::pair<int, int>>> result;
        result.assign(n, {});

        size_t numberOfJourneys = 0;

        size_t i(0);
        for (const StopQuery& query : queries) {
            algorithm.run(query.source, query.departureTime, query.target);
            numberOfJourneys += algorithm.getJourneys().size();
            result[i].reserve(algorithm.getArrivals().size());

            /*             std::cout << "MLTB Query" << std::endl; */
            /*             for (auto& journey : algorithm.getJourneys()) { */
            /*                 std::cout << query << std::endl; */
            /*                 for (auto& leg : journey) { */
            /*                     std::cout << (int)leg.from << " -> " << (int)leg.to << " @ " << leg.departureTime << " -> " << leg.arrivalTime << (leg.usesRoute ? ", route: " : ", transfer: ") << (int)leg.routeId; */
            /*                     if (!leg.usesRoute && Edge(leg.routeId) != noEdge) { */
            /*                         uint8_t lcl = std::min( */
            /*                             data.getLowestCommonLevel(StopId(leg.from), query.source), */
            /*                             data.getLowestCommonLevel(StopId(leg.from), query.target)); */
            /*                         std::cout << " LocalLevel: " << (int)data.stopEventGraph.get(LocalLevel, Edge(leg.routeId)) << " and lcl: " << (int)lcl; */
            /*                     } */

            /*                     std::cout << std::endl; */
            /*                 } */
            /*                 std::cout << std::endl; */
            /*             } */

            for (auto& arr : algorithm.getArrivals()) {
                result[i].push_back(std::make_pair(arr.numberOfTrips, arr.arrivalTime));
            }

            i += 1;
        }
        algorithm.getProfiler().printStatistics();
        std::cout << "Avg. Journeys: " << String::prettyDouble(numberOfJourneys / (float)queries.size()) << std::endl;
        algorithm.showTransferLevels();

        if (eval) {
            size_t wrongQueries = 0;
            std::cout << "Evaluation against TB:" << std::endl;
            TripBased::Data trip(evalFile);
            trip.printInfo();
            TripBased::TransitiveQuery<TripBased::AggregateProfiler> tripAlgorithm(trip);
            std::vector<std::vector<std::pair<int, int>>> tripResult;
            tripResult.assign(n, {});

            numberOfJourneys = 0;
            i = 0;
            for (const StopQuery& query : queries) {
                tripAlgorithm.run(query.source, query.departureTime, query.target);
                numberOfJourneys += tripAlgorithm.getJourneys().size();

                /*                 std::cout << "TB Query" << std::endl; */
                /*                 for (auto& journey : tripAlgorithm.getJourneys()) { */
                /*                     std::cout << query << std::endl; */
                /*                     for (auto& leg : journey) { */
                /*                         std::cout << (int)leg.from << " -> " << (int)leg.to << " @ " << leg.departureTime << " -> " << leg.arrivalTime << (leg.usesRoute ? ", route: " : ", transfer: ") << (int)leg.routeId; */
                /*                         if (!leg.usesRoute && Edge(leg.routeId) != noEdge) { */
                /*                             uint8_t lcl = std::min( */
                /*                                 data.getLowestCommonLevel(StopId(leg.from), query.source), */
                /*                                 data.getLowestCommonLevel(StopId(leg.from), query.target)); */
                /*                             std::cout << " LocalLevel: " << (int)data.stopEventGraph.get(LocalLevel, Edge(leg.routeId)) << " and lcl: " << (int)lcl; */
                /*                         } */

                /*                         std::cout << std::endl; */
                /*                     } */
                /*                     std::cout << std::endl; */
                /*                 } */

                tripResult[i].reserve(tripAlgorithm.getArrivals().size());

                for (auto& arr : tripAlgorithm.getArrivals()) {
                    tripResult[i].push_back(std::make_pair(arr.numberOfTrips, arr.arrivalTime));
                }

                i += 1;
            }
            tripAlgorithm.getProfiler().printStatistics();
            std::cout << "Avg. Journeys: " << String::prettyDouble(numberOfJourneys / (float)queries.size()) << std::endl;

            for (size_t i(0); i < queries.size(); ++i) {
                // computes the results from TB, which are not in MLTB
                std::set<std::pair<int, int>> set1(tripResult[i].begin(), tripResult[i].end());
                std::set<std::pair<int, int>> set2(result[i].begin(), result[i].end());
                std::set<std::pair<int, int>> difference;
                std::set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(),
                    std::inserter(difference, difference.begin()));

                if (difference.size() > 0) {
                    ++wrongQueries;
                    std::cout << "Query: " << queries[i] << std::endl;
                }
            }

            std::cout << "Wrong queries: " << wrongQueries << std::endl;
        }
    }
};

class WriteMLTBToCSV : public ParameterizedCommand {
public:
    WriteMLTBToCSV(BasicShell& shell)
        : ParameterizedCommand(shell, "writeMLTBToCSV", "Writes MLTB Data to csv files")
    {
        addParameter("Input file (MLTB Data)");
        addParameter("Output file (CSV files)");
    }

    virtual void execute() noexcept
    {
        const std::string mltb = getParameter("Input file (MLTB Data)");
        const std::string output = getParameter("Output file (CSV files)");

        TripBased::MLData data(mltb);
        data.printInfo();

        data.raptorData.writeCSV(output);
        data.writePartitionToCSV(output + "partition.csv");

        Graph::toEdgeListCSV(output + "transfer", data.stopEventGraph);
    }
};
