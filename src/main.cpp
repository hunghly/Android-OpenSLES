#include <iostream>
#include <unistd.h>
#include <string.h>
#include "opensl.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

int main(int argc, char **argv)
{
    int c;
    int counter;
    int max_seconds = -1;
    char filepath[100];
    char filename[100];

    if (argc < 3) {
        std::cout << "Incorrect number of arguments" << std::endl;
        std::cout << "Usage: ./opensl-recorder -s <# of seconds> [-f <filepath>]" << std::endl;
        std::cout << "<# of seconds> must range from the values 10 - 900" << std::endl;
        return -1;
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
                std::cout << "Usage: ./opensl-recorder -s <# of seconds> [-f <filepath>]" << std::endl;
                std::cout << "<# of seconds> must range from the values 10 - 900" << std::endl;
                break;
        }
    }

    if (max_seconds == -1) {
        std::cout << "Missing -s flag" << std::endl;
        std::cout << "Usage: ./opensl-recorder -s <# of seconds> [-f <filepath>]" << std::endl;
        std::cout << "<# of seconds> must range from the values 10 - 900" << std::endl;
        return -1;
    } else if (max_seconds < 10 || max_seconds > 900) {
        std::cout << "-s argument out of bounds" << std::endl;
        std::cout << "Usage: ./opensl-recorder -s <# of seconds> [-f <filepath>]" << std::endl;
        std::cout << "<# of seconds> must range from the values 10 - 900" << std::endl;
        return -1;
    }

    std::cout << "Creating Engine, Audio Player, and Recorder objects" << std::endl;
    LOGD("Creating Engine, Audio Player, and Recorder objects");
    create_engine();
    create_buffer_queue_audio_player(48000, 512); // currently setting output to 48KHz
    create_recorder();

    /* Need to use a sleep here to delay next execution because SetDurationLimit 
     * was not working during testing.
     */
    std::cout << "Starting Recorder" << std::endl;
    LOGD("Starting Recorder");
    set_recording_state(START_RECORD);
    counter = max_seconds;
    while (counter > 0) {
        counter--;
        std::cout << "Recording time remaining: " << counter << " seconds" << std::endl;
        sleep(1);
    }

    std::cout << "Stopping Recorder" << std::endl;
    LOGD("Stopping Recorder");
    set_recording_state(STOP_RECORD);

    std::cout << "Starting Playback" << std::endl;
    LOGD("Starting Playback");
    play_clip();
    counter = max_seconds;
    while (counter > 0) {
        counter--;
        std::cout << "Playback time remaining: " << counter << " seconds" << std::endl;
        sleep(1);
    }

    // destroy audio recorder object, and invalidate all associated interfaces
    LOGD("Cleaning up");
    std::cout << "Cleaning up" << std::endl;
    shutdown();

    return 1;
}