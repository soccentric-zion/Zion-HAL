#include "test_runner.h"
#include "security_features_lib/security_features_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SecurityFeaturesTesting>("SecurityFeaturesTesting", argc, argv);
}
