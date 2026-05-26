#include "test_runner.h"
#include "pcie_lib/pcie_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<PcieTesting>("PcieTesting", argc, argv);
}
