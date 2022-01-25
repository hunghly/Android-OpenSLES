#include <iostream>
#include "opensl.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SLES/OpenSLES.h> // OpenSLES header
#include <SLES/OpenSLES_Android.h> // OenSLES Android extension

int main(int argc, char **argv)
{
    int c;
    int counter;
    int max_seconds = -1;
    char filepath[100];
    char filename[100];

    if (argc < 3) {
        std::cout << "Incorrect number of arguments" << std::endl;
        std::cout << "Usage: test-audio -s <# of seconds> [-f <filepath>]" << std::endl;
        return 0;
    }
    while ((c=getopt(argc, argv, "?s:f:")) != -1) {
        switch (c) {
            case 's':
                max_seconds = atoi(optarg);
                break;
            case 'f':
                strncpy(filename, optarg, strlen(optarg) + 1);
                break;
            case '?':
                std::cout << "\nInvalid flag found\n" << std::endl;
                std::cout << "Usage: test-audio -s <# of seconds> [-f <filepath>]" << std::endl;
                break;
        }
    }
    if (max_seconds == -1) {
        std::cout << "Missing -s flag" << std::endl;
        std::cout << "Usage: test-audio -s <# of seconds> [-f <filepath>]" << std::endl;
        return 0;
    }
    std::cout << "Filename: " << filename << std::endl;
    std::cout << "Seconds: " << max_seconds << std::endl;

    std::cout << "Starting Audio Binary" << std::endl;
    LOGD("Starting Audio Binary");

    std::cout << "Creating Engine, Audio Player, and Recorder objects" << std::endl;
    LOGD("Creating Engine, Audio Player, and Recorder objects");
    create_engine();
    create_buffer_queue_audio_player(48000, 512);
    create_recorder();

    std::cout << "Starting Recorder" << std::endl;
    LOGD("Starting Recorder");
    set_recording_state(START_RECORD);
    counter = max_seconds;
    while (counter > 0) {
        counter--;
        std::cout << "Recording time remaining: " << counter << " seconds" << std::endl;
        sleep(1);
    }
    // sleep(10);
    std::cout << "Stopping Recorder" << std::endl;
    LOGD("Stopping Recorder");
    set_recording_state(STOP_RECORD);

    std::cout << "Starting Playback" << std::endl;
    LOGD("Starting Playback");
    select_clip(4, 1);

    counter = max_seconds;
    while (counter > 0) {
        counter--;
        std::cout << "Playback time remaining: " << counter << " seconds" << std::endl;
        sleep(1);
    }

    // clean up
    // destroy audio recorder object, and invalidate all associated interfaces
    LOGD("Cleaning up");
    std::cout << "Cleaning up" << std::endl;
    shutdown();

    return 0;
}