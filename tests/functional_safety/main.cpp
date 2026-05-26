#include "test_runner.h"
#include "functional_safety_lib/functional_safety_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<FunctionalSafetyTesting>("FunctionalSafetyTesting", argc, argv);
}
