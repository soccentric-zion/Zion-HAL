#include "test_runner.h"
#include "system_memory_lib/system_memory_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SystemMemoryTesting>("SystemMemoryTesting", argc, argv);
}
