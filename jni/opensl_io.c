#include "opensl_io.h"
#include <stdio.h>
#include <assert.h>

static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
circular_buffer* create_circular_buffer(int bytes);
int checkspace_circular_buffer(circular_buffer *p, int writeCheck);
int read_circular_buffer_bytes(circular_buffer *p, short *out, int bytes);
int write_circular_buffer_bytes(circular_buffer *p, const short *in, int bytes);
void free_circular_buffer(circular_buffer *p);

const SLEnvironmentalReverbSettings reverbSettings =
SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;

//char * file_name = "/sdcard/opensl_record_native.pcm";
//char * file_name1 = "/sdcard/opensl_record_java.pcm";
//FILE *fp;
//FILE *fp1;

// creates the OpenSL ES audio engine
static SLresult openSLCreateEngine(OPENSL_STREAM *p) {
	SLresult result;
	// create engine
	result = slCreateEngine(&(p->engineObject), 0, NULL, 0, NULL, NULL);
	assert(SL_RESULT_SUCCESS == result);

	// realize the engine
	result = (*p->engineObject)->Realize(p->engineObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);

	// get the engine interface, which is needed in order to create other objects
	result = (*p->engineObject)->GetInterface(p->engineObject, SL_IID_ENGINE,
			&(p->engineEngine));
	assert(SL_RESULT_SUCCESS == result);

//	fp = fopen(file_name, "wb");
//	fp1 = fopen(file_name1, "wb");

	return result;
}

// opens the OpenSL ES device for output
static SLresult openSLPlayOpen(OPENSL_STREAM *p) {
	SLresult result;
	SLuint32 sr = p->sr;
	SLuint32 channels = p->outchannels;

	if (channels) {
		// configure audio source
		SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
		sr = SL_SAMPLINGRATE_8;

		const SLInterfaceID ids[] = { SL_IID_ENVIRONMENTALREVERB };
		const SLboolean req[] = { SL_BOOLEAN_TRUE };
		result = (*p->engineEngine)->CreateOutputMix(p->engineEngine,
				&(p->outputMixObject), 1, ids, req);
		assert(SL_RESULT_SUCCESS == result);

		// set environmental reverb
		result = (*p->outputMixObject)->GetInterface(p->outputMixObject,
				SL_IID_ENVIRONMENTALREVERB, &(p->outputMixEnvironmentalReverb));
		if (SL_RESULT_SUCCESS == result) {
			result =
					(*p->outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
							p->outputMixEnvironmentalReverb, &reverbSettings);
			assert(SL_RESULT_SUCCESS == result);
		}

		// realize the output mix
		result = (*p->outputMixObject)->Realize(p->outputMixObject,
		SL_BOOLEAN_FALSE);
		assert(SL_RESULT_SUCCESS == result);

		int speakers;
		speakers = SL_SPEAKER_FRONT_CENTER;

		SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, channels, sr,
		SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, speakers,
		SL_BYTEORDER_LITTLEENDIAN };

		SLDataSource audioSrc = { &loc_bufq, &format_pcm };

		// configure audio sink
		SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX,
				p->outputMixObject };
		SLDataSink audioSnk = { &loc_outmix, NULL };

		// create audio player
		const SLInterfaceID ids1[] = { SL_IID_ANDROIDCONFIGURATION,
				SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
		const SLboolean req1[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
		SL_BOOLEAN_TRUE };
		result = (*p->engineEngine)->CreateAudioPlayer(p->engineEngine,
				&(p->bqPlayerObject), &audioSrc, &audioSnk, 3, ids1, req1);
		assert(SL_RESULT_SUCCESS == result);

		// My code
		SLAndroidConfigurationItf playerConfig;
		result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject,
				SL_IID_ANDROIDCONFIGURATION, &playerConfig);
		assert(SL_RESULT_SUCCESS == result);
		SLint32 streamType = SL_ANDROID_STREAM_VOICE;
		result = (*playerConfig)->SetConfiguration(playerConfig,
		SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
		assert(SL_RESULT_SUCCESS == result);
		// End my code

		// realize the player
		result = (*p->bqPlayerObject)->Realize(p->bqPlayerObject,
		SL_BOOLEAN_FALSE);
		assert(SL_RESULT_SUCCESS == result);

		// get the play interface
		result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject,
				SL_IID_PLAY, &(p->bqPlayerPlay));
		assert(SL_RESULT_SUCCESS == result);

		// get the buffer queue interface
		result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject,
				SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(p->bqPlayerBufferQueue));
		assert(SL_RESULT_SUCCESS == result);

		// register callback on the buffer queue
		result = (*p->bqPlayerBufferQueue)->RegisterCallback(
				p->bqPlayerBufferQueue, bqPlayerCallback, p);
		assert(SL_RESULT_SUCCESS == result);

		// get the volume interface
		result = (*p->bqPlayerObject)->GetInterface(p->bqPlayerObject,
				SL_IID_VOLUME, &(p->bqPlayerVolume));
		assert(SL_RESULT_SUCCESS == result);

		// set the player's state to playing
		result = (*p->bqPlayerPlay)->SetPlayState(p->bqPlayerPlay,
		SL_PLAYSTATE_PLAYING);
		assert(SL_RESULT_SUCCESS == result);

		if ((p->playBuffer = (short *) calloc(p->outBufSamples, sizeof(short)))
				== NULL) {
			return -1;
		}

		(*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue,
				p->playBuffer, p->outBufSamples * sizeof(short));

		return result;
	}
	return SL_RESULT_SUCCESS;
}

// Open the OpenSL ES device for input
static SLresult openSLRecOpen(OPENSL_STREAM *p, int SDK) {

	SLresult result;
	SLuint32 sr = p->sr;
	SLuint32 channels = p->inchannels;
	SLint32 streamType;

//	LOGD("",channels);

	if (channels) {

		sr = SL_SAMPLINGRATE_8;

		// configure audio source
		SLDataLocator_IODevice loc_dev = { SL_DATALOCATOR_IODEVICE,
		SL_IODEVICE_AUDIOINPUT,
		SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
		SLDataSource audioSrc = { &loc_dev, NULL };

		// configure audio sink
		int speakers;
		speakers = SL_SPEAKER_FRONT_CENTER;

		SLDataLocator_AndroidSimpleBufferQueue loc_bq = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
		SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, channels, sr,
		SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, speakers,
		SL_BYTEORDER_LITTLEENDIAN };
		SLDataSink audioSnk = { &loc_bq, &format_pcm };

		// create audio recorder
		// (requires the RECORD_AUDIO permission)
		const SLInterfaceID id[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
				SL_IID_ANDROIDCONFIGURATION, SL_IID_VOLUME };
		const SLboolean req[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
		SL_BOOLEAN_TRUE };
		result = (*p->engineEngine)->CreateAudioRecorder(p->engineEngine,
				&(p->recorderObject), &audioSrc, &audioSnk, 2, id, req);
		assert(SL_RESULT_SUCCESS == result);

		// My code
		SLAndroidConfigurationItf recorderConfig;
		result = (*p->recorderObject)->GetInterface(p->recorderObject,
				SL_IID_ANDROIDCONFIGURATION, &recorderConfig);
		assert(SL_RESULT_SUCCESS == result);

		// This setting is important. Need to check Android Version
		if (SDK < 14)
			streamType = SL_ANDROID_RECORDING_PRESET_GENERIC;
		//If android API level is >= 14
		else
			streamType = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;

		result = (*recorderConfig)->SetConfiguration(recorderConfig,
		SL_ANDROID_KEY_RECORDING_PRESET, &streamType, sizeof(SLint32));
		assert(SL_RESULT_SUCCESS == result);
		// My recorder configuration code ends.

		// realize the audio recorder
		result = (*p->recorderObject)->Realize(p->recorderObject,
		SL_BOOLEAN_FALSE);
		assert(SL_RESULT_SUCCESS == result);

		// get the record interface
		result = (*p->recorderObject)->GetInterface(p->recorderObject,
				SL_IID_RECORD, &(p->recorderRecord));
		assert(SL_RESULT_SUCCESS == result);

		// get the buffer queue interface
		result = (*p->recorderObject)->GetInterface(p->recorderObject,
				SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(p->recorderBufferQueue));
		assert(SL_RESULT_SUCCESS == result);

		// register callback on the buffer queue
		result = (*p->recorderBufferQueue)->RegisterCallback(
				p->recorderBufferQueue, bqRecorderCallback, p);
		assert(SL_RESULT_SUCCESS == result);
		result = (*p->recorderRecord)->SetRecordState(p->recorderRecord,
		SL_RECORDSTATE_RECORDING);
		assert(SL_RESULT_SUCCESS == result);

		if ((p->recBuffer = (short *) calloc(p->inBufSamples, sizeof(short)))
				== NULL) {
			return -1;
		}

		(*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue, p->recBuffer,
				p->inBufSamples * sizeof(short));

		return result;
	} else
		return SL_RESULT_SUCCESS;
}

// close the OpenSL IO and destroy the audio engine
static void openSLDestroyEngine(OPENSL_STREAM *p) {

// destroy buffer queue audio player object, and invalidate all associated interfaces
	if (p->bqPlayerObject != NULL) {
		SLuint32 state = SL_PLAYSTATE_PLAYING;
		(*p->bqPlayerPlay)->SetPlayState(p->bqPlayerPlay, SL_PLAYSTATE_STOPPED);
		while (state != SL_PLAYSTATE_STOPPED)
			(*p->bqPlayerPlay)->GetPlayState(p->bqPlayerPlay, &state);
		(*p->bqPlayerObject)->Destroy(p->bqPlayerObject);
		p->bqPlayerObject = NULL;
		p->bqPlayerPlay = NULL;
		p->bqPlayerBufferQueue = NULL;
		p->bqPlayerEffectSend = NULL;
	}

// destroy audio recorder object, and invalidate all associated interfaces
	if (p->recorderObject != NULL) {
		SLuint32 state = SL_PLAYSTATE_PLAYING;
		(*p->recorderRecord)->SetRecordState(p->recorderRecord,
		SL_RECORDSTATE_STOPPED);
		while (state != SL_RECORDSTATE_STOPPED)
			(*p->recorderRecord)->GetRecordState(p->recorderRecord, &state);
		(*p->recorderObject)->Destroy(p->recorderObject);
		p->recorderObject = NULL;
		p->recorderRecord = NULL;
		p->recorderBufferQueue = NULL;
	}

// destroy output mix object, and invalidate all associated interfaces
	if (p->outputMixObject != NULL) {
		(*p->outputMixObject)->Destroy(p->outputMixObject);
		p->outputMixObject = NULL;
	}

// destroy engine object, and invalidate all associated interfaces
	if (p->engineObject != NULL) {
		(*p->engineObject)->Destroy(p->engineObject);
		p->engineObject = NULL;
		p->engineEngine = NULL;
	}

//	fclose(fp);
//	fclose(fp1);

}
// open the android audio device for input and/or output
OPENSL_STREAM *android_OpenAudioDevice(int sr, int inchannels, int outchannels,
		int bufferframes, int SDK) {

	OPENSL_STREAM * p;
	p = (OPENSL_STREAM *) malloc(sizeof(OPENSL_STREAM));
	memset(p, 0, sizeof(OPENSL_STREAM));
	p->inchannels = inchannels;
	p->outchannels = outchannels;
	p->sr = sr;
	p->outBufSamples = bufferframes * outchannels;
	p->inBufSamples = bufferframes * inchannels;

	if ((p->outrb = create_circular_buffer(p->outBufSamples * 10)) == NULL) {
		android_CloseAudioDevice(p);
		return NULL ;
	}
	if ((p->inrb = create_circular_buffer(p->inBufSamples * 10)) == NULL) {
		android_CloseAudioDevice(p);
		return NULL ;
	}

	if (openSLCreateEngine(p) != SL_RESULT_SUCCESS) {
		android_CloseAudioDevice(p);
		return NULL ;
	}

	if (openSLRecOpen(p, SDK) != SL_RESULT_SUCCESS) {
		android_CloseAudioDevice(p);
		return NULL ;
	}

	if (openSLPlayOpen(p) != SL_RESULT_SUCCESS) {
		android_CloseAudioDevice(p);
		return NULL ;
	}

	return p;
}

// close the android audio device
void android_CloseAudioDevice(OPENSL_STREAM *p) {

	if (p == NULL)
		return;

	openSLDestroyEngine(p);

	if (p->playBuffer != NULL) {
		free(p->playBuffer);
		p->playBuffer = NULL;
	}

	if (p->recBuffer != NULL) {
		free(p->recBuffer);
		p->recBuffer = NULL;
	}

	free_circular_buffer(p->inrb);
	free_circular_buffer(p->outrb);

	free(p);
}

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
	OPENSL_STREAM *p = (OPENSL_STREAM *) context;
	int bytes = p->inBufSamples * sizeof(short);
	write_circular_buffer_bytes(p->inrb, p->recBuffer, p->inBufSamples);
//	fwrite(p->recBuffer, sizeof(short), p->inBufSamples, fp);

	(*p->recorderBufferQueue)->Enqueue(p->recorderBufferQueue, p->recBuffer,
			bytes);
}

// gets a buffer of size samples from the device
int android_AudioIn(OPENSL_STREAM *p, short *buffer, int size) {
	if (p == NULL || p->inBufSamples == 0)
		return 0;
	int shorts = read_circular_buffer_bytes(p->inrb, buffer, size);

//	if(shorts)
//		fwrite(buffer, sizeof(short), size, fp1);

	return shorts;
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
	OPENSL_STREAM *p = (OPENSL_STREAM *) context;
	int bytes = p->outBufSamples * sizeof(short);
	int shorts = read_circular_buffer_bytes(p->outrb, p->playBuffer,
			p->outBufSamples);
//	LOGD("shorts read during bqPlayerCallback: %d", shorts);
	if(!shorts)
		memset(p->playBuffer, 0, bytes);

	(*p->bqPlayerBufferQueue)->Enqueue(p->bqPlayerBufferQueue,
				p->playBuffer, bytes);
}

// puts a buffer of size samples to the device
int android_AudioOut(OPENSL_STREAM *p, short *buffer, int size) {
	if (p == NULL || p->outBufSamples == 0)
		return 0;
	int shorts = write_circular_buffer_bytes(p->outrb, buffer, size);
	return shorts;
}

circular_buffer* create_circular_buffer(int shorts) {
	circular_buffer * p;
	if ((p = calloc(1, sizeof(circular_buffer))) == NULL) {
		return NULL ;
	}
	p->size = shorts;
	p->wp = p->rp = 0;

	if ((p->buffer = calloc(shorts, sizeof(short))) == NULL) {
		free(p);
		return NULL ;
	}
	return p;
}

int checkspace_circular_buffer(circular_buffer *p, int writeCheck) {
	int wp = p->wp, rp = p->rp, size = p->size;
	if (writeCheck) {
		if (wp > rp)
			return rp - wp + size - 1;
		else if (wp < rp)
			return rp - wp - 1;
		else
			return size - 1;
	} else {
		if (wp > rp)
			return wp - rp;
		else if (wp < rp)
			return wp - rp + size;
		else
			return 0;
	}
}

int read_circular_buffer_bytes(circular_buffer *p, short *out, int shorts) {
	int remaining;
	int shortsread, size = p->size, rp = p->rp;
	short *buffer = p->buffer;
	if ((remaining = checkspace_circular_buffer(p, 0)) == 0) {
		return 0;
	}
	shortsread = shorts > remaining ? remaining : shorts;
	if (rp + shortsread < size) {
		memcpy(out, buffer + rp, shortsread * sizeof(short));
		//memset(buffer + rp, 0, shortsread * sizeof(short));
		p->rp = shortsread + rp;
	} else {
		memcpy(out, buffer + rp, (size - rp) * sizeof(short));
		//memset(buffer + rp, 0, (size - rp) * sizeof(short));
		memcpy(out + (size - rp), buffer,
				(shortsread + rp - size) * sizeof(short));
		//memset(buffer, 0, (shortsread + rp - size) * sizeof(short));
		p->rp = shortsread + rp - size;
	}

	return shortsread;
}

int write_circular_buffer_bytes(circular_buffer *p, const short *in, int shorts) {
	int remaining;
	int shortswrite, size = p->size, wp = p->wp;
	short *buffer = p->buffer;
	if ((remaining = checkspace_circular_buffer(p, 1)) == 0) {
		return 0;
	}
	shortswrite = shorts > remaining ? remaining : shorts;
	if (wp + shortswrite < size) {
		memcpy(buffer + wp, in, shortswrite * sizeof(short));
		p->wp = wp + shortswrite;
	} else {
		memcpy(buffer + wp, in, (size - wp) * sizeof(short));
		memcpy(buffer, in + (size - wp),
				(shortswrite + wp - size) * sizeof(short));
		p->wp = shortswrite + wp - size;
	}

	return shortswrite;
}

void free_circular_buffer(circular_buffer *p) {
	if (p == NULL)
		return;
	free(p->buffer);
	free(p);
}
