#include "test_runner.h"
#include "gnss_lib/gnss_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<GnssTesting>("GnssTesting", argc, argv);
}
