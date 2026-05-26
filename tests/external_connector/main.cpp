#include "test_runner.h"
#include "external_connector_lib/external_connector_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<ExternalConnectorTesting>("ExternalConnectorTesting", argc, argv);
}
