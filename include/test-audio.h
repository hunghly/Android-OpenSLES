#ifndef TESTAUDIO_H
#define TESTAUDIO_H

#include <android/log.h>
#include <SLES/OpenSLES.h> // OpenSLES header
#include <SLES/OpenSLES_Android.h> // OenSLES Android extension

#define LOGTAG "OPENSL-ES"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, __VA_ARGS__)
#define OPENSL_ES_FAIL 0
#define OPENSL_ES_SUCCESS 1


// signatures
// void run_recorder(int frames);
void create_engine();
int create_recorder();
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
void set_recording_state();
void select_clip(int which, int count);
short* createResampledBuf(uint32_t idx, uint32_t srcRate, unsigned *size);
void create_buffer_queue_audio_player(int sampleRate, int bufSize);
void releaseResampleBuf(void);
void shutdown();

#endif