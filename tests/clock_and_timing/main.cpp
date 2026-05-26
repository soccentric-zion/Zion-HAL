#include "test_runner.h"
#include "clock_and_timing_lib/clock_and_timing_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<ClockAndTimingTesting>("ClockAndTimingTesting", argc, argv);
}
