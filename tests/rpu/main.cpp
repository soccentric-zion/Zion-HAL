#include "test_runner.h"
#include "rpu_lib/rpu_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<RpuTesting>("RpuTesting", argc, argv);
}