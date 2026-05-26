#include "test_runner.h"
#include "ethernet_lib/ethernet_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<EthernetTesting>("EthernetTesting", argc, argv);
}
