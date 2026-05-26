#include "test_runner.h"
#include "watchdog_and_reset_lib/watchdog_and_reset_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<WatchdogAndResetTesting>("WatchdogAndResetTesting", argc, argv);
}
