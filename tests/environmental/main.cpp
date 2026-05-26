#include "test_runner.h"
#include "environmental_lib/environmental_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<EnvironmentalTesting>("EnvironmentalTesting", argc, argv);
}
