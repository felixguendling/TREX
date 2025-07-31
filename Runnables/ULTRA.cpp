/**********************************************************************************

 Copyright (c) 2023-2025 Patrick Steil
 Copyright (c) 2019-2022 KIT ITI Algorithmics Group

 MIT License

 Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************************/
#include "../Helpers/Console/CommandLineParser.h"
#include "../Shell/Shell.h"
#include "Commands/CH.h"
#include "Commands/QueryBenchmark.h"
#include "Commands/ULTRAPreprocessing.h"
using namespace Shell;

int main(int argc, char** argv) {
  CommandLineParser clp(argc, argv);
  pinThreadToCoreId(clp.value<int>("core", 1));
  checkAsserts();
  ::Shell::Shell shell;
  new BuildCH(shell);
  new BuildCoreCH(shell);

  new ComputeStopToStopShortcuts(shell);
  new ComputeMcStopToStopShortcuts(shell);
  new ComputeMultimodalMcStopToStopShortcuts(shell);
  new RAPTORToTripBased(shell);
  new ComputeEventToEventShortcuts(shell);
  new ComputeTransitiveEventToEventShortcuts(shell);
  new ComputeMcEventToEventShortcuts(shell);
  new ComputeMultimodalMcEventToEventShortcuts(shell);
  new AugmentTripBasedShortcuts(shell);
  new ValidateStopToStopShortcuts(shell);
  new ValidateEventToEventShortcuts(shell);

  new RunTransitiveCSAQueries(shell);
  new RunTransitiveProfileCSAQueries(shell);
  new RunDijkstraCSAQueries(shell);
  new RunHLCSAQueries(shell);
  new RunULTRACSAQueries(shell);

  new RunTransitiveRAPTORQueries(shell);
  new RunDijkstraRAPTORQueries(shell);
  new RunHLRAPTORQueries(shell);
  new RunULTRARAPTORQueries(shell);

  new RunTransitiveMcRAPTORQueries(shell);
  new RunMCRQueries(shell);
  new RunULTRAMcRAPTORQueries(shell);
  new RunTransitiveBoundedMcRAPTORQueries(shell);
  new RunUBMRAPTORQueries(shell);

  new RunTransitiveTripBasedQueries(shell);
  new RunULTRATripBasedQueries(shell);

  new RunULTRAMcTripBasedQueries(shell);
  new RunBoundedULTRAMcTripBasedQueries(shell);

  new RunMultimodalMCRQueries(shell);
  new RunMultimodalULTRAMcRAPTORQueries(shell);
  new RunUBMHydRAQueries(shell);
  new RunMultimodalUBMRAPTORQueries(shell);
  new RunMultimodalUBMHydRAQueries(shell);

  new ComputeTransferTimeSavings(shell);
  shell.run();
  return 0;
}
