#include "test_runner.h"
#include "communication_interface_lib/communication_interface_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<CommunicationInterfaceTesting>("CommunicationInterfaceTesting", argc, argv);
}
