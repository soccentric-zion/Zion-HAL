#include "test_runner.h"
#include "serializer_deserializer_lib/serializer_deserializer_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<SerializerDeserializerTesting>("SerializerDeserializerTesting", argc, argv);
}
