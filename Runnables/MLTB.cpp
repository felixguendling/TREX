#include "Commands/MLTB.h"
#include "../Helpers/Console/CommandLineParser.h"
#include "../Shell/Shell.h"

using namespace Shell;

int main(int argc, char** argv)
{
    CommandLineParser clp(argc, argv);
    pinThreadToCoreId(clp.value<int>("core", 1));
    checkAsserts();
    ::Shell::Shell shell;

    new ApplyPartitionFile(shell);
    new RAPTORToMLTB(shell);
    new CreateCompactLayoutGraph(shell);
    new Customization(shell);
    new ShowInfoOfMLTB(shell);
    new RunMLQuery(shell);

    shell.run();
    return 0;
}
