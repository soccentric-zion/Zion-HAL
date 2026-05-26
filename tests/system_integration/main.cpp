#include "test_runner.h"
#include "system_integration_lib/system_integration_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SystemIntegrationTesting>("SystemIntegrationTesting", argc, argv);
}
