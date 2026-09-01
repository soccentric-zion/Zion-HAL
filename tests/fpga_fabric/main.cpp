#include "test_runner.h"
#include "fpga_fabric_lib/fpga_fabric_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<FpgaFabricTesting>("FpgaFabricTesting", argc, argv);
}