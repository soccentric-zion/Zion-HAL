#include "test_runner.h"
#include "imu_lib/imu_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<ImuTesting>("ImuTesting", argc, argv);
}
