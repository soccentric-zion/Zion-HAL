#include "test_runner.h"
#include "gpu_lib/gpu_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<GpuTesting>("GpuTesting", argc, argv);
}
