#include "test_runner.h"
#include "software_interface_lib/software_interface_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SoftwareInterfaceTesting>("SoftwareInterfaceTesting", argc, argv);
}
