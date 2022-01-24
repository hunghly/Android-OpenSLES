#include <iostream>
#include "test-audio.h"
#include <unistd.h>
#include <stdlib.h>

// a mutext to guard against re-entrance to record & playback
// as well as make recording and playing back to be mutually exclusive
// this is to avoid crash at situations like:
//    recording is in session [not finished]
//    user presses record button and another recording coming in
// The action: when recording/playing back is not finished, ignore the new request
static pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

// engine interfaces
static SLEngineItf engineEngine; // This interface exposes creation methods of all the OpenSL ES object types.
static SLObjectItf engineObject = NULL; //The SLObjectItf interface provides essential utility methods for all objects. 

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// recorder interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
    SL_I3DL2_ENVIRONMENT_PRESET_ROOM;

// 10 seconds of recorded audio at 16 kHz mono, 16-bit signed little endian
#define RECORDER_FRAMES (16000 * 10)
static short recorderBuffer[RECORDER_FRAMES];
// static short *recorderBuffer;
static unsigned recorderSize = 0;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLVolumeItf bqPlayerVolume;
static SLmilliHertz bqPlayerSampleRate = 0;
static short *resampleBuf = NULL;
static int   bqPlayerBufSize = 0;

void run_recorder(int frames) {
    int seconds = frames;
    // #undef RECORDER_FRAMES
    // #define RECORDER_FRAMES (16000 * 30)
    // recorderBuffer = (short*) malloc(16000 * seconds);

    std::cout << "frames:" << RECORDER_FRAMES << std::endl;
    std::cout << "seconds:" << seconds << std::endl;

    std::cout << "size of recorder buffer" << sizeof(recorderBuffer) << std::endl;
    // create_engine();
    // create_buffer_queue_audio_player(48000, 512);
    // create_recorder();
    // set_recording_state();
    // sleep(100);
    // select_clip(4, 1);
    // sleep(100);

    // clean up
    // destroy audio recorder object, and invalidate all associated interfaces
    LOGD("Cleaning up");
    std::cout << "Cleaning up" << std::endl;
    free(recorderBuffer);
    // shutdown();
}


void create_engine()
{
    LOGD("Creating Audio Engine");
    std::cout << "Creating Audio Engine" << std::endl;

    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
            &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example
    LOGD("No Issues");
    std::cout << "No Issues" << std::endl;
}

// create audio recorder: recorder is not in fast path
//    like to avoid excessive re-sampling while playing back from Hello & Android clip
int create_recorder()
{
    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc,
            &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
        LOGD("Error creating audio recorder");
        std::cout << "Error creating audio recorder" << std::endl;
        return OPENSL_ES_FAIL;
    }

    // realize the audio recorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        LOGD("Error realizing audio recorder");
        std::cout << "Error realizing audio recorder" << std::endl;
        return OPENSL_ES_FAIL;
    }

    // get the record interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            &recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    LOGD("Registering Recorder Callback");
    std::cout << "Registering Recorder Callback" << std::endl;
    // register callback on the buffer queue
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback,
            NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    return OPENSL_ES_SUCCESS;
}

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    std::cout << "Recording Callback Triggered" << std::endl;
    LOGD("Recording Callback Triggered");
    assert(bq == recorderBufferQueue);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize = RECORDER_FRAMES * sizeof(short);
    }
    pthread_mutex_unlock(&audioEngineLock);
}

// set the recording state for the audio recorder
void set_recording_state()
{
    SLresult result;

    if (pthread_mutex_trylock(&audioEngineLock)) {
        std::cout << "Unable to unlock mutex lock" << std::endl;
        return;
    }
    // in case already recording, stop recording and clear buffer queue
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*recorderBufferQueue)->Clear(recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // the buffer is not valid for playback yet
    recorderSize = 0;

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, recorderBuffer,
            RECORDER_FRAMES * sizeof(short));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
    (void)result;


    // Set the duration of the recording - 30 seconds (30,000 milliseconds)
    result = (*recorderRecord)->SetDurationLimit(recorderRecord, 3000);
    std::cout << "Setting duration to 2 seconds" << std::endl;

    assert(SL_RESULT_SUCCESS == result);
    LOGD("Starting recording...");
    std::cout << "Starting recording..." << std::endl;
    // start recording
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    std::cout << "Result is: " << result << std::endl;
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}

// select the desired clip and play count, and enqueue the first buffer if idle
void select_clip(int which, int count)
{        
    LOGD("Selecting clip...");
    std::cout << "Selecting clip..." << std::endl;
    if (pthread_mutex_trylock(&audioEngineLock)) {
        // If we could not acquire audio engine lock, reject this request and client should re-try
        // return 1;
    }
    switch (which) {
        case 4:     // CLIP_PLAYBACK
            LOGD("Created resampled buffer... Next Size %d", nextSize);
            std::cout << "Creating resampled buffer... Next Size:" << nextSize << std::endl;
            nextBuffer = createResampledBuf(4, SL_SAMPLINGRATE_16, &nextSize);
            // we recorded at 16 kHz, but are playing buffers at 8 Khz, so do a primitive down-sample
            if(!nextBuffer) {
                unsigned i;
                for (i = 0; i < recorderSize; i += 2 * sizeof(short)) {
                    recorderBuffer[i >> 2] = recorderBuffer[i >> 1];
                }
                recorderSize >>= 1;
                nextBuffer = recorderBuffer;
                nextSize = recorderSize;
                std::cout << "recorderSize:" << recorderSize << " nextBuffer: " << nextBuffer << " nextsize: " << nextSize << std::endl;
            }
            break;
        default:
            nextBuffer = NULL;
            nextSize = 0;
            break;
    }
    nextCount = count;
    if (nextSize > 0) {
        LOGD("Playing back audio...");
        std::cout << "Playing back audio..." << std::endl;
        // here we only enqueue one buffer because it is a long clip,
        // but for streaming playback we would typically enqueue at least 2 buffers to start
        SLresult result;
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        if (SL_RESULT_SUCCESS != result) {
            pthread_mutex_unlock(&audioEngineLock);
            // return JNI_FALSE;
        }
    } else {
        pthread_mutex_unlock(&audioEngineLock);
    }

    // return 0;
}

/*
 * Only support up-sampling
 */
short* createResampledBuf(uint32_t idx, uint32_t srcRate, unsigned *size) {
    short  *src = NULL;
    short  *workBuf;
    int    upSampleRate;
    int32_t srcSampleCount = 0;
    std::cout << "Inside of create resampledbuffer...1" << std::endl;

    if(0 == bqPlayerSampleRate) {
        std::cout << "Inside of create resampledbuffer...2" << std::endl;

        return NULL;
    }
    if(bqPlayerSampleRate % srcRate) {
        /*
         * simple up-sampling, must be divisible
         */
        std::cout << "Inside of create resampledbuffer...3" << std::endl;
        return NULL;
    }
    upSampleRate = bqPlayerSampleRate / srcRate;
    std::cout << "Inside of create resampledbuffer..." << "upsample:" << upSampleRate << std::endl;

    switch (idx) {
        case 4: // captured frames
            std::cout << "Case 4" << std::endl;
            srcSampleCount = recorderSize / sizeof(short);
            src =  recorderBuffer;
            break;
        default:
            assert(0);
            return NULL;
    }

    resampleBuf = (short*) malloc((srcSampleCount * upSampleRate) << 1);
    if(resampleBuf == NULL) {
        return resampleBuf;
    }
    workBuf = resampleBuf;
    for(int sample=0; sample < srcSampleCount; sample++) {
        for(int dup = 0; dup  < upSampleRate; dup++) {
            *workBuf++ = src[sample];
        }
    }

    *size = (srcSampleCount * upSampleRate) << 1;     // sample format is 16 bit
    std::cout << "Returning resampled buffer: " << resampleBuf << std::endl;
    return resampleBuf;
}

// create buffer queue audio player
void create_buffer_queue_audio_player(int sampleRate, int bufSize)
{
    SLresult result;
    LOGD("Creating BQAP");
    std::cout << "Creating Buffer Queue Audio Player" << std::endl;
    if (sampleRate >= 0 && bufSize >= 0 ) {
        bqPlayerSampleRate = sampleRate * 1000;
        /*
         * device native buffer size is another factor to minimize audio latency, not used in this
         * sample: we only play one giant buffer here
         */
        bqPlayerBufSize = bufSize;
    }

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if(bqPlayerSampleRate) {
        format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
                                    /*SL_IID_MUTESOLO,*/};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
                                   /*SL_BOOLEAN_TRUE,*/ };

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
            bqPlayerSampleRate? 2 : 3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
            &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    bqPlayerEffectSend = NULL;
    if( 0 == bqPlayerSampleRate) {
        result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                                 &bqPlayerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;


    // Get duration
    SLmillisecond      DurationMsec = 0;
    result = (*bqPlayerPlay)->GetDuration(bqPlayerPlay, &DurationMsec);
    if (DurationMsec != SL_TIME_UNKNOWN) {
        std::cout << "Duration is: " << DurationMsec << std::endl;
    } else {
        std::cout << "Duration is SL_TIME_UNKNOWN" << std::endl;
    }


    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    LOGD("Inside of PlayerCallback");
    std::cout << "Inside of Player Callback" << std::endl;

    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (--nextCount > 0 && NULL != nextBuffer && 0 != nextSize) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        if (SL_RESULT_SUCCESS != result) {
            pthread_mutex_unlock(&audioEngineLock);
        }
        (void)result;
    } else {
        releaseResampleBuf();
        pthread_mutex_unlock(&audioEngineLock);
    }
}

void releaseResampleBuf(void) {
    if( 0 == bqPlayerSampleRate) {
        /*
         * we are not using fast path, so we were not creating buffers, nothing to do
         */
        return;
    }

    free(resampleBuf);
    resampleBuf = NULL;
}

void shutdown()
{

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        // bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }

    // // destroy file descriptor audio player object, and invalidate all associated interfaces
    // if (fdPlayerObject != NULL) {
    //     (*fdPlayerObject)->Destroy(fdPlayerObject);
    //     fdPlayerObject = NULL;
    //     fdPlayerPlay = NULL;
    //     fdPlayerSeek = NULL;
    //     fdPlayerMuteSolo = NULL;
    //     fdPlayerVolume = NULL;
    // }

    // // destroy URI audio player object, and invalidate all associated interfaces
    // if (uriPlayerObject != NULL) {
    //     (*uriPlayerObject)->Destroy(uriPlayerObject);
    //     uriPlayerObject = NULL;
    //     uriPlayerPlay = NULL;
    //     uriPlayerSeek = NULL;
    //     uriPlayerMuteSolo = NULL;
    //     uriPlayerVolume = NULL;
    // }

    // destroy audio recorder object, and invalidate all associated interfaces
    if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recorderRecord = NULL;
        recorderBufferQueue = NULL;
    }

    // // destroy output mix object, and invalidate all associated interfaces
    // if (outputMixObject != NULL) {
    //     (*outputMixObject)->Destroy(outputMixObject);
    //     outputMixObject = NULL;
    //     outputMixEnvironmentalReverb = NULL;
    // }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    pthread_mutex_destroy(&audioEngineLock);
}