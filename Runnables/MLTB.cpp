#include "Commands/MLTB.h"

#include "../Helpers/Console/CommandLineParser.h"
#include "../Shell/Shell.h"
#include "Commands/NetworkIO.h"
#include "Commands/NetworkTools.h"
#include "Commands/QueryBenchmark.h"

using namespace Shell;

int main(int argc, char **argv) {
  std::cout << std::endl;
  std::cout << "░        ░░       ░░░        ░░  ░░░░  ░\n";
  std::cout << "▒▒▒▒  ▒▒▒▒▒  ▒▒▒▒  ▒▒  ▒▒▒▒▒▒▒▒▒  ▒▒  ▒▒\n";
  std::cout << "▓▓▓▓  ▓▓▓▓▓       ▓▓▓      ▓▓▓▓▓▓    ▓▓▓\n";
  std::cout << "████  █████  ███  ███  █████████  ██  ██\n";
  std::cout << "████  █████  ████  ██        ██  ████  █\n";
  std::cout << std::endl;

  CommandLineParser clp(argc, argv);
  pinThreadToCoreId(clp.value<int>("core", 1));
  checkAsserts();
  ::Shell::Shell shell;

  new ApplyPartitionFile(shell);
  new RAPTORToMLTB(shell);
  new CreateCompactLayoutGraph(shell);
  new Customization(shell);
  new ShowInfoOfMLTB(shell);
  new WriteMLTBToCSV(shell);
  new EventDistributionOverTime(shell);
  new CheckBorderStops(shell);
  new ExportMLTBTimeExpandedGraph(shell);
  new BuildTBTEGraph(shell);
  new ShowInducedCellOfNetwork(shell);

  new RunMLQuery(shell);
  new RunMLTBProfileQueries(shell);

  new RunTransitiveRAPTORQueries(shell);
  new RunOneTransitiveRAPTORQuery(shell);
  new RunTransitiveTripBasedQueries(shell);
  new RunTransitiveCSAQueries(shell);
  new RunTransitiveProfileTripBasedQueries(shell);

  new RunGeoRankedRAPTORQueries(shell);
  new RunGeoRankedTripBasedQueries(shell);
  new RunGeoRankedMLTBQueries(shell);

  new IntermediateToTD(shell);
  new IntermediateToTE(shell);

  new ExportTEGraphToHubLabelFile(shell);

  new RunTDDijkstraQueries(shell);
  new RunTEDijkstraQueries(shell);

  new TEToPTL(shell);
  new RunPTLQueries(shell);

  new DistanceNetwork(shell);
  new StopsImportance(shell);

  shell.run();
  return 0;
}
