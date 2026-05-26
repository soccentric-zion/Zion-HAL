#include "test_runner.h"
#include "mcu_interface_lib/mcu_interface_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<McuInterfaceTesting>("McuInterfaceTesting", argc, argv);
}
