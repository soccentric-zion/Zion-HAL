#include "test_runner.h"
#include "thermal_management_lib/thermal_management_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<ThermalManagementTesting>("ThermalManagementTesting", argc, argv);
}
