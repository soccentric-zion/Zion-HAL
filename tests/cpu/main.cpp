#include "test_runner.h"
#include "cpu_lib/cpu_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<CpuTesting>("CpuTesting", argc, argv);
}
