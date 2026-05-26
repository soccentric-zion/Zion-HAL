#include "test_runner.h"
#include "video_encode_decode_lib/video_encode_decode_lib.h"

int main(int argc, char* argv[])
{
    return runTestApp<VideoEncodeDecodeTesting>("VideoEncodeDecodeTesting", argc, argv);
}
