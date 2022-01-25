#ifndef OPENSL_H
#define OPENSL_H

#include <android/log.h>
#include <SLES/OpenSLES.h> // OpenSLES header
#include <SLES/OpenSLES_Android.h> // OenSLES Android extension

#define LOGTAG "OPENSL-ES"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, __VA_ARGS__)
#define OPENSL_FAIL 0
#define OPENSL_SUCCESS 1
#define START_RECORD 1
#define STOP_RECORD 0

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
static SLRecordItf recorderItf;
static SLAndroidSimpleBufferQueueItf recorderBufferQueue;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
    SL_I3DL2_ENVIRONMENT_PRESET_ROOM;

// 15 minutes of recorded audio at 16 kHz mono, 16-bit signed little endian
#define RECORDER_FRAMES (16000 * 900)
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

void create_engine();
int create_recorder();
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
void set_recording_state(int state);
int play_clip();
short* createResampledBuf(uint32_t srcRate, unsigned *size);
void create_buffer_queue_audio_player(int sampleRate, int bufSize);
void releaseResampleBuf(void);
void shutdown();

#endif