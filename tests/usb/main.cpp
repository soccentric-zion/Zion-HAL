#include "test_runner.h"
#include "usb_lib/usb_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<UsbTesting>("UsbTesting", argc, argv);
}
