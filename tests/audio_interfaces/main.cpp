#include "test_runner.h"
#include "audio_interfaces/audio_interfaces.h"

int main(int argc, char* argv[])
{
    return runTestApp<AudioInterfaces>("AudioInterfaces", argc, argv);
}
