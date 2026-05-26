#include "test_runner.h"
#include "reliability_lib/reliability_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<ReliabilityTesting>("ReliabilityTesting", argc, argv);
}
