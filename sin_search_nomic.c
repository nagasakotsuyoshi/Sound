
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>

#include <alsa/asoundlib.h>
#include <math.h>
#include "inputKey.h"
#include <unistd.h>
#include <time.h>
static char *device = "plughw:1,0";                        /* playback device */
snd_output_t *output = NULL;
//#define BUFSIZE 32*1024
#define BUFSIZE 65536*2*2*2
#define NCHAN 2
#define SMPLFREQ 192000		/* sample rate */
int16_t buffer[BUFSIZE];                          /* some random data */

#define PI 3.141592
enum BOOL{ TRUE = 1, FALSE = 0 };
static enum BOOL bRunning = FALSE;


int main(int argc, char *argv[]) {
  int err;
  unsigned int i;
  unsigned int samplerates=0;
  snd_pcm_t *handle;
  snd_pcm_sframes_t avail;
  snd_pcm_sframes_t frames;

  snd_pcm_sframes_t frames_capture;


  /* sin wave */
  double theta;
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
    Vol[i]=2000*(i+1);
  }
  
  nsample = SMPLFREQ / freq;
  deltatheta = PI *2 / nsample;
  
  
  /* playback */
  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  if ((err = snd_pcm_set_params(handle,
				SND_PCM_FORMAT_S16_LE,
				SND_PCM_ACCESS_RW_INTERLEAVED,
				NCHAN,
				SMPLFREQ,
				1,
				500000)) < 0) {   /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }


  
#define ABS(x) ((x < 0) ? -x : x)
  

  int cmd = 1;
  init_keyboard();
  unsigned int volcnt,thetacnt,bufcnt;          //volume count,theta count
  for(freq;freq<490;freq+=10){
    nsample = SMPLFREQ / freq;
    deltatheta = PI *2 / nsample;
    theta = 0;
    for(bufcnt=0;bufcnt<BUFSIZE/NCHAN;bufcnt+=2){
	  
      buffer[bufcnt] = sin(theta)*Vol[volcnt];
      buffer[bufcnt+1] = sin(theta)*30000;
      theta += deltatheta;
    }
    
    printf("freq:%d\ndeltatheta=%f nsample = %d\n", freq,deltatheta,nsample);
    
    for (thetacnt=180;thetacnt < nsample;thetacnt++){
      theta = thetacnt * deltatheta;

      //      printf("theta%d\n",thetacnt);
      for (volcnt = 0; volcnt < voln; volcnt++) {
	
	//	printf("now volume: %d\n",Vol[volcnt]);
	

	
	for(bufcnt=0;bufcnt<BUFSIZE/NCHAN;bufcnt+=2){
	  
	  buffer[bufcnt] = sin(theta)*Vol[volcnt];

	  theta += deltatheta;

	}
	printf("buffer : %ld\n",sizeof(buffer)/sizeof(buffer[0]));


	frames = snd_pcm_writei(handle, buffer,BUFSIZE/NCHAN);

 

	
        printf("vol: %d ,theta : %d ,",Vol[volcnt],thetacnt);
	

	err = snd_pcm_wait(handle,-1);
	if(err < 0){
	  if((err = snd_pcm_recover(handle,err,0))<0){
	    printf("pcm wait error: %s",snd_strerror(err));
	  }
	}
	
	InputKey(buffer,&freq,&nsample,&cmd);


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
  
  return 0;
}
	//printf("buffer : %ld\n",sizeof(buffer)/sizeof(buffer[0]));
