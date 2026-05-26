#include "test_runner.h"
#include "ethernet_switch_lib/ethernet_switch_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<EthernetSwitchTesting>("EthernetSwitchTesting", argc, argv);
}
