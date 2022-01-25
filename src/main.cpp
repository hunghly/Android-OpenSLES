#include <iostream>
#include "test-audio.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h> // OpenSLES header
#include <SLES/OpenSLES_Android.h> // OenSLES Android extension


int main(int argc, char **argv)
{
    int c;
    int seconds = 0;
    char filepath[100];
    char filename[100];
    if (argc < 2) {
        std::cout << "Defaulting to 10 second recording" << std::endl;
    }
    while ((c=getopt(argc, argv, "?s:f:")) != -1) {
        switch (c) {
            case 's':
                seconds = atoi(optarg);
                printf("seconds: %d", seconds);
                // recorderBuffer[RECORDER_FRAMES];
            case 'p':
                strncpy(filepath, optarg, strlen(optarg) + 1);
            case 'f':
                strncpy(filename, optarg, strlen(optarg) + 1);
                break;
            case '?':
                printf("Invalid flag found\n");
                break;
        }
    }
    LOGD("Starting Audio Binary");
    // run_recorder(seconds);
    // create_engine();
    // create_buffer_queue_audio_player(48000, 512);
    // create_recorder();
    // set_recording_state();
    // sleep(100);
    // select_clip(4, 1);
    // sleep(100);

    // clean up
    // destroy audio recorder object, and invalidate all associated interfaces
    // LOGD("Cleaning up");
    // std::cout << "Cleaning up" << std::endl;
    // shutdown();

    return 0;
}