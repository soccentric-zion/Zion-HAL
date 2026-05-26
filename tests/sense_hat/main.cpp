#include "test_runner.h"
#include "sense_hat_lib/sense_hat_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SenseHatTesting>("SenseHatTesting", argc, argv);
}
