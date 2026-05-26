#include "test_runner.h"
#include "gpio_lib/gpio_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<GpioTesting>("GpioTesting", argc, argv);
}
