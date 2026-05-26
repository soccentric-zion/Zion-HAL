#include "test_runner.h"
#include "vision_accelerator_lib/vision_accelerator_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<VisionAcceleratorTesting>("VisionAcceleratorTesting", argc, argv);
}
