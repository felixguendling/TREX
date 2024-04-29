#include <iostream>
#include <string>

#include "../../DataStructures/Graph/Graph.h"
#include "NestedBiPartitioner.h"

int main(int argn, char** argv)
{
    if (argn != 4) {
        std::cout << "Wrong number of arguments!\nCall: " << argv[0] << " COMPACT_LAYOUT_GRAPH NUMBER_OF_LEVELS OUTPUT_FILENAME" << std::endl;
        return -1;
    }
    const std::string graphFileName = argv[1];
    const int numLevels = std::stoi(argv[2]);
    const std::string outputFileName = argv[3];

    std::cout << "Reading a graph from file '" << graphFileName << "' and compute a nested partition of " << numLevels << "!\n";
    StaticGraphWithWeightsAndCoordinatesAndSize compactLayoutGraph(graphFileName);

    size_t numVertices = compactLayoutGraph.numVertices();
    size_t numEdges = compactLayoutGraph.numEdges();

    std::vector<size_t> toAdj(numVertices + 1);
    std::vector<size_t> toVertex(numEdges);
    std::vector<size_t> toWeight(numEdges);
    std::vector<size_t> vertexWeight(numVertices);

    size_t runningSum(0);

    for (Vertex from : compactLayoutGraph.vertices()) {
        toAdj[from] = runningSum;
        vertexWeight[from] = compactLayoutGraph.get(Size, from);

        for (Edge edge : compactLayoutGraph.edgesFrom(from)) {
            toVertex[runningSum] = compactLayoutGraph.get(ToVertex, edge);
            toWeight[runningSum] = compactLayoutGraph.get(Weight, edge);

            ++runningSum;
        }
    }
    toAdj.back() = runningSum;

    Partitioner part(toAdj, toVertex, toWeight, vertexWeight, numLevels);
    part.startNestedBipartition();
    part.writePartitionToFile(outputFileName);
}
