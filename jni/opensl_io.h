#ifndef OPENSL_IO
#define OPENSL_IO


#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>


#include <stdlib.h>
#include <unistd.h>

#include <android/log.h>

#define TAG "OPENSL IO"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

typedef struct _circular_buffer {
	short *buffer;
	int wp;
	int rp;
	int size;
} circular_buffer;

typedef struct opensl_stream {

	// engine interfaces
	SLObjectItf engineObject;
	SLEngineItf engineEngine;

	// output mix interfaces
	SLObjectItf outputMixObject;
	SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

	// buffer queue player interfaces
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
	SLEffectSendItf bqPlayerEffectSend;
	SLVolumeItf bqPlayerVolume;

	// recorder interfaces
	SLObjectItf recorderObject;
	SLRecordItf recorderRecord;
	SLAndroidSimpleBufferQueueItf recorderBufferQueue;


	// buffers
//	short *outputBuffer;
//	short *inputBuffer;
	short *recBuffer;
	short *playBuffer;

	circular_buffer *outrb;
	circular_buffer *inrb;

	// size of buffers
	int outBufSamples;
	int inBufSamples;

//	double time;
	int inchannels;
	int outchannels;
	int sr;

} OPENSL_STREAM;

/*
 Open the audio device with a given sampling rate (sr), input and output channels and IO buffer size
 in frames. Returns a handle to the OpenSL stream
 */
OPENSL_STREAM* android_OpenAudioDevice(int sr, int inchannels, int outchannels,
		int bufferframes, int SDK);
/*
 Close the audio device
 */
void android_CloseAudioDevice(OPENSL_STREAM *p);
/*
 Read a buffer from the OpenSL stream *p, of size samples. Returns the number of samples read.
 */
int android_AudioIn(OPENSL_STREAM *p, short *buffer, int size);
/*
 Write a buffer to the OpenSL stream *p, of size samples. Returns the number of samples written.
 */
int android_AudioOut(OPENSL_STREAM *p, short *buffer, int size);

#endif // #ifndef OPENSL_IO
