#include "test_runner.h"
#include "poe_hat_lib/poe_hat_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<PoeHatTesting>("PoeHatTesting", argc, argv);
}
