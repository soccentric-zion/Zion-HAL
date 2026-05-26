#include "test_runner.h"
#include "display_interfaces/display_interfaces.h"

int main(int argc, char* argv[])
{
    return runTestApp<DisplayInterfaces>("DisplayInterfaces", argc, argv);
}
