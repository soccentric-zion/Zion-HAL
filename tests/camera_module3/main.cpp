#include "test_runner.h"
#include "camera_module3_lib/camera_module3_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<CameraModule3Testing>("CameraModule3Testing", argc, argv);
}
