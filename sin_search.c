
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
/* pcm  */
#include <alsa/asoundlib.h>
#include <math.h>
#include "inputKey.h"
#include <unistd.h>
#include <time.h>
/* thread */
#include <err.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>



static char *device = "plughw:1,0";                        /* playback device */
snd_output_t *output = NULL;
//#define BUFSIZE 32*1024
#define BUFSIZE 65536*2
#define NCHAN 2
#define SMPLFREQ 192000		/* sample rate */
int16_t buffer[BUFSIZE];                          /* some random data */

#define PI 3.141592
enum BOOL{ TRUE = 1, FALSE = 0 };
static enum BOOL bRunning = FALSE;

typedef struct {
  snd_pcm_t *handle;
  int16_t buffer[BUFSIZE];
  int frames;
  char *capture_buffer;
}T_SOUND;


#define ABS(x) ((x < 0) ? -x : x)

static snd_pcm_t* PreparePCM(int hwid, snd_pcm_format_t format, unsigned int* rate)
{
  
    int sound_err;
    snd_pcm_t* capture_handle = NULL;
    snd_pcm_hw_params_t* hw_params;
    char input[32]; sprintf(input,"plughw:%d",hwid);//hw:1
    
    if ( (sound_err = snd_pcm_open(&capture_handle,input,SND_PCM_STREAM_CAPTURE,0)) < 0 ) {
        printf("error: open audio device %s (%s)\n",input,snd_strerror(sound_err)); return NULL;
    }
    printf("audio interface opened\n");
    if ( (sound_err = snd_pcm_hw_params_malloc(&hw_params)) < 0 ) {
        printf("error: allocate hardware with parameter (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params allocated\n");
    if ( (sound_err = snd_pcm_hw_params_any(capture_handle,hw_params)) < 0 ) {
        printf("error: initialize hardware parameter structure (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params initialized\n");
    if ( (sound_err = snd_pcm_hw_params_set_access(capture_handle,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ) {
        printf("error: set access type (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params access setted\n");
    if ( (sound_err = snd_pcm_hw_params_set_format(capture_handle,hw_params,format)) < 0 ) {
        printf("error: set sample format (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params format setted\n");
    if ( (sound_err = snd_pcm_hw_params_set_rate_near(capture_handle,hw_params,rate,0)) < 0 ) {
        printf("error: set sample rate (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    snd_pcm_hw_params_get_rate(hw_params,rate,NULL);
    
    printf("hw_params rate setted\n");
    if ( (sound_err = snd_pcm_hw_params_set_channels(capture_handle,hw_params,1)) < 0 ) {
        printf("error: set channel count (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }

    printf("hw_params channels setted\n");
    if ( (sound_err = snd_pcm_hw_params(capture_handle,hw_params)) < 0 ) {
        printf("error: set parameters (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    snd_pcm_hw_params_free(hw_params);
    
    if ( (sound_err = snd_pcm_prepare(capture_handle)) < 0 ) {
        printf("error: prepare audio interface for use (%s)\n",snd_strerror(sound_err));
        snd_pcm_close(capture_handle); return NULL;
    }
    return capture_handle;
}

void * Malloc(size_t size){
  void *p = malloc(size);
  if(NULL == p){
    err(EXIT_FAILURE,"malloc error:%lu",size);
  }
  return p;
}

void * sound_output(void *pb){
  int sound_err;
  void *frames = Malloc(sizeof(snd_pcm_sframes_t));
  T_SOUND *ipb;
  ipb = pb;
  *(snd_pcm_sframes_t*)(frames) = snd_pcm_writei(ipb->handle, ipb->buffer,ipb->frames);

  sound_err = snd_pcm_wait(ipb->handle,100);	  

  return frames;
}

void * amplitude(void *ct){

  usleep(1*100000);
  printf("start record");
  int sound_err;
  T_SOUND *ict;
  ict = ct;
  int ci,cj,ck;	      /* capture i,j,k */
  short *sp;


  //  int sum_cj=0,i;
  int nSamplesPerSec = (ict->frames * 16 * 2) / 8;
  int length = ict->frames * snd_pcm_format_width(SND_PCM_FORMAT_S16_LE) / 8;
  
  if ( (sound_err = snd_pcm_readi(ict->handle,ict->capture_buffer,ict->frames)) != ict->frames ) {
    printf("%d: read from audio interface failed (%s)\n",sound_err,snd_strerror(sound_err));
    return 0;
  }

  //  for(i = 0;i<10;i++){
    ck = length/2;		/* 16bit=2byte */
    sp = ict->capture_buffer;
    cj=0;
    for(ci = 0;ci<ck;ci++){
      cj+=ABS(sp[ci]);
    }
    //   sum_cj+=cj;
    //  }
  void *avg_cj =  Malloc(sizeof(int));
  *(int*)(avg_cj) = (int)cj/(ck/1);
  printf("sound : %d\n",cj/(ck/1));

  return avg_cj;
}


int main(int argc, char *argv[]) {
  int sound_err;
  unsigned int i,j;
  unsigned int samplerates=0;
  snd_pcm_t *handle;
  snd_pcm_sframes_t frames;
  unsigned int smplfreq = SMPLFREQ;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  snd_pcm_t *capture_handle = PreparePCM(0,format,&smplfreq);

  int min_amplitude = 30000,inserveted_theta=0,min_volume=0;
  
  /* sin wave */
  double theta = 0;
  double deltatheta;

  int freq = 480;
  int nsample;
  int voln;

  if (argc >= 3) {
    freq = atoi(argv[1]);
    voln = atoi(argv[2]);
    printf("FREQ=%d\n", freq);
    printf("change volume times :%d\n",voln); 
  }else{
    printf("nazenaze\n");
    voln = 10;
  }
  
  int Vol[voln];
  for (i = 0; i < voln;i++){
    Vol[i]=3000*(i+1)/voln;
  }
  
  nsample = SMPLFREQ / freq;
  deltatheta = PI *2 / nsample;
  
  
  /* playback */
  if ((sound_err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(sound_err));
    exit(EXIT_FAILURE);
  }
  if ((sound_err = snd_pcm_set_params(handle,
				SND_PCM_FORMAT_S16_LE,
				SND_PCM_ACCESS_RW_INTERLEAVED,
				NCHAN,
				SMPLFREQ,
				1,
				500000)) < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(sound_err));
    exit(EXIT_FAILURE);
  }
  /* caputure */
   int buffer_frames = 48000,length = buffer_frames * snd_pcm_format_width(SND_PCM_FORMAT_S16_LE) / 8;
  char* capture_buffer = (char*)malloc(length);
  printf("audio interface prepared: %d, length - %d\n",snd_pcm_format_width(format),length);


  /* input key */
  int cmd = 1;
  init_keyboard();
  unsigned int volcnt,thetacnt,bufcnt;

  /* start sound search */
  T_SOUND playback_info,capture_info;
  
  for(freq;freq<500;freq+=10){
    theta = 0;
    nsample = SMPLFREQ / freq;
    deltatheta = PI *2 / nsample;    
    printf("freq:%d\ndeltatheta=%f nsample = %d\n", freq,deltatheta,nsample);

    for(bufcnt=0;bufcnt<BUFSIZE/NCHAN;bufcnt+=2){
      buffer[bufcnt] = sin(theta)*Vol[volcnt];
      buffer[bufcnt+1] = sin(theta)*30000;
      theta += deltatheta;
    }
    
    for (thetacnt=180;thetacnt < 240;thetacnt++){
      theta=thetacnt*deltatheta;

      for (volcnt = 0; volcnt < voln; volcnt++) {




	/* thread */
	pthread_t pplayback,pcapture;
	int ret1,ret2;
	void *pb_status,*ct_status;
	const char *retc1,*retc2;
	int k;
	playback_info.handle = handle;
	for(j = 0;j < sizeof(buffer)/sizeof(buffer[0]);j++){
	  playback_info.buffer[j] = buffer[j];
	}

	for(k = 0;k<10;k++){
	playback_info.frames = BUFSIZE/NCHAN;

	
        ret1 = pthread_create(&pplayback,NULL,(void *)sound_output,(void *)&playback_info);
	capture_info.handle = capture_handle;
	capture_info.capture_buffer = capture_buffer;
	capture_info.frames = buffer_frames;

        ret2 = pthread_create(&pcapture,NULL,(void *)amplitude,(void *)&capture_info);
 
        if (ret1 != 0) {
                err(EXIT_FAILURE, "can not create thread 1: %s", strerror(ret1) );
        }
	
        if (ret2 != 0) {
                err(EXIT_FAILURE, "can not create thread 2: %s", strerror(ret2) );
        }
 


        ret1 = pthread_join(pplayback,&pb_status);
        if (ret1 != 0) {
                errx(EXIT_FAILURE, retc1, "can not join thread 1");
        }
 
        ret2 = pthread_join(pcapture,&ct_status);
        if (ret2 != 0) {
                errx(EXIT_FAILURE, retc2, "can not join thread 2");
        }
 
        printf("done\n");
	printf("playback frames:%d capture amplitude:%d\n",* (int*)pb_status,*(int*)ct_status);
	frames = * (snd_pcm_sframes_t*)pb_status;
	
	
	  printf("vol: %d ,theta : %d ,",Vol[volcnt],thetacnt);

	  /* while(1){ */
	  /*   if(sound_err = snd_pcm_wait(capture_handle,100000)) */
	  /*     break; */
	  /* } */
	  sound_err = snd_pcm_wait(capture_handle,100000);

	  snd_pcm_avail_update(capture_handle);
	  sound_err = snd_pcm_recover(capture_handle,sound_err,0);
	  /* printf("snd_pcm_readi failed : %s\n",snd_strerror(sound_err)); */

	  /* input key  */
	InputKey(buffer,&freq,&nsample,&cmd);
	  
	  /* playback error */
	if (frames < 0)
	  frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
	  printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
	  break;
	}
	if (frames > 0 && frames < (long)2*nsample/NCHAN)
	  printf("Short write (expected %li, wrote %li)\n", (long)2*nsample/NCHAN, frames);
	if(cmd == 0){
	  break;
	}
	free(pb_status);
	free(ct_status);
	}// k
      }//volcnt
      if(cmd == 0){
	break;
      }
    }//thetacnt
    if(cmd == 0){
      break;
    }
  }//freq



  snd_pcm_drain(handle);
  snd_pcm_close(handle);

  free(capture_handle);
  snd_pcm_close(capture_handle);
  free(capture_buffer);
  return 0;
}
