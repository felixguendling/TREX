#include "Commands/MLTB.h"
#include "../Helpers/Console/CommandLineParser.h"
#include "../Shell/Shell.h"
#include "Commands/QueryBenchmark.h"

using namespace Shell;

int main(int argc, char** argv)
{

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
    new RunMLQuery(shell);
    new RunTransitiveRAPTORQueries(shell);
    new RunTransitiveTripBasedQueries(shell);
    new RunTransitiveCSAQueries(shell);
    new RunTransitiveProfileTripBasedQueries(shell);

    new RunGeoRankedRAPTORQueries(shell);

    shell.run();
    return 0;
}
