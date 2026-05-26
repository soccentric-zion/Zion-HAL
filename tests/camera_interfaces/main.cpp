#include "test_runner.h"
#include "camera_interfaces_lib/camera_interfaces_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<CameraInterfacesTesting>("CameraInterfacesTesting", argc, argv);
}
