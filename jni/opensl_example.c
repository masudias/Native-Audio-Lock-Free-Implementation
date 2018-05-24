#include <android/log.h>
#include "opensl_io.h"

#define VECSAMPS_MONO 1600
#define SR 8000
//#define MICROS 1000000

static int on;
void start_process(int SDK) {
 OPENSL_STREAM *p;
 int samps, i, j;
 short inbuffer[VECSAMPS_MONO], outbuffer[VECSAMPS_MONO];
 p = android_OpenAudioDevice(SR,1,1,VECSAMPS_MONO, SDK);
 if(p == NULL) return;
 on = 1;
 while(on) {
 samps = android_AudioIn(p,inbuffer,VECSAMPS_MONO);

 //usleep(MICROS*VECSAMPS_MONO/SR);

 for(i=0;i<samps;i++) outbuffer[i] = inbuffer[i];

 android_AudioOut(p,outbuffer,samps);
 }
 android_CloseAudioDevice(p);
}

void stop_process(){
 on = 0;
}
