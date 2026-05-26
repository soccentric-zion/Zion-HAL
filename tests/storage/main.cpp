#include "test_runner.h"
#include "storage_lib/storage_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<StorageTesting>("StorageTesting", argc, argv);
}
