#include "test_runner.h"
#include "debug_and_development_interfaces/debug_and_development_interfaces.h"

int main(int argc, char* argv[])
{
    return runTestApp<DebugAndDevelopmentInterfaces>("DebugAndDevelopmentInterfaces", argc, argv);
}
