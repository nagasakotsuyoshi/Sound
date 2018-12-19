/*
 *  This ehxtra small demo sends a random samples to your speakers.
 */
#include <alsa/asoundlib.h>
#include <math.h>
#include "micget.h"
#include <unistd.h>
#include <time.h>
static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
//#define BUFSIZE 32*1024
#define BUFSIZE 65536*2
#define NCHAN 2
#define SMPLFREQ 48000		/* sample rate */
int16_t buffer[BUFSIZE];                          /* some random data */
#define PI 3.141592
int main(int argc, char *argv[]) {
  int err;
  unsigned int i;
  unsigned int samplerates=0;
  snd_pcm_t *handle;
  snd_pcm_sframes_t frames;


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
    Vol[i]=30000*(i+1)/voln;
  }
  
  nsample = SMPLFREQ / freq;
  deltatheta = PI *2 / nsample;
  
  printf("deltatheta=%f nsample = %d\n", deltatheta,nsample);
  for (i = 0; i < nsample*2 ; i+=2) {
    buffer[i] = sin(theta)*Vol[0];
    buffer[i+1] = sin(theta)*30000;
    theta += deltatheta;
  }
  
  
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
  
  clock_t start,end;
  unsigned int volcnt,thetacnt;
  for(freq=480;freq<500;freq+=10){
    nsample = SMPLFREQ / freq;
    deltatheta = PI *2 / nsample;
    
    printf("freq:%d\ndeltatheta=%f nsample = %d\n", freq,deltatheta,nsample);
    
    for (thetacnt=0;thetacnt < nsample;thetacnt++){
      theta=thetacnt*deltatheta;
      printf("theta%d\n",thetacnt);
      for (volcnt = 0; volcnt < voln; volcnt++) {
	
	printf("now volume: %d\n",Vol[volcnt]);
	
	int bufcnt;
	
	for(bufcnt=0;bufcnt<65536*2/2;bufcnt+=2){
	  
	  buffer[bufcnt] = sin(theta)*Vol[volcnt];
	  buffer[bufcnt+1] = sin(theta)*30000;
	  theta += deltatheta;
	}
	printf("buffer : %ld\n",sizeof(buffer)/sizeof(buffer[0]));
	
	frames = snd_pcm_writei(handle, buffer,65536*2/NCHAN);
	//100*2*nsample
	
	//record();
	
	printf("frames:%ld\n",frames);
	
	if(snd_pcm_delay(handle,&frames)==0){
	  printf("success\n");
	}
	//samplerates = 0;
	//	snd_pcm_drain(handle);
	/*	int k;
		for(k=0;k < 20;k++){
		if(err = snd_pcm_writei(handle,buffer,100*2*nsample/NCHAN)!= 100*2*nsample/NCHAN){
		printf("%d\n",err);
		exit(1);
		}
		//samplerates += sample rate
		
		}
	*/
	
	  while(1){
	    if(err = snd_pcm_wait(handle,100000))
	      printf("%d\n",err);
	    if(err == 1)
	      break;
	    
	  }

	  printf("avail:%ld\n",snd_pcm_avail_update(handle));

	

	if (frames < 0)
	  frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
	  printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
	  break;
	}
	if (frames > 0 && frames < (long)2*nsample/NCHAN)
	  printf("Short write (expected %li, wrote %li)\n", (long)2*nsample/NCHAN, frames);
	
	
      }//volcnt
      
    }//thetacnt
  }//freq
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  
  //close_keyboard();
  return 0;
}
