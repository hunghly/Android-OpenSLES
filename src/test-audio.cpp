#include <iostream>
#include "test-audio.h"

#include <SLES/OpenSLES.h> // OpenSLES header
#include <SLES/OpenSLES_Android.h> // OenSLES Android extension


// The two headers below are part of the extension and not needed
// #include <SLES/OpenSLES_AndroidConfiguration.h>
// #include <SLES/OpenSLES_AndroidMetadata.h>

int main()
{
    LOGD("Starting Audio Binary");
    std::cout << "Hello World!!" << std::endl;
    return 0;
}