/*
 *  This extra small demo sends a random samples to your speakers.
 */
#include <alsa/asoundlib.h>
#include <math.h>
#include "inputKey.h"
#include <unistd.h>
static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
// #define BUFSIZE 16*1024
#define BUFSIZE 65536
#define NCHAN 2
#define SMPLFREQ 48000		/* sample rate */
int16_t buffer[BUFSIZE];                          /* some random data */
#define PI 3.141592
int main(int argc, char *argv[]) {
  int err;
  unsigned int i;
  snd_pcm_t *handle;
  snd_pcm_sframes_t frames;
  double theta = 0;
  double deltatheta;

  int freq = 480;
  int nsample;

  if (argc >= 2) {
    freq = atoi(argv[1]);
    printf("FREQ=%d\n", freq);
  }else{
    printf("nazenaze\n");
  }


  nsample = SMPLFREQ / freq;
  deltatheta = PI *2 / nsample;
  
  printf("deltatheta=%f nsample = %d\n", deltatheta,nsample);
  for (i = 0; i < nsample*2 ; i+=2) {
    buffer[i] = sin(theta)*30000;
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
  //for (i = 0; i < 16; i++) {

  int cmd = 1;
  init_keyboard();
    while (1) {

    frames = snd_pcm_writei(handle, buffer, 2*nsample/NCHAN);
    InputKey(buffer,&freq,&nsample,&cmd);
    
    if (frames < 0)
      frames = snd_pcm_recover(handle, frames, 0);
    if (frames < 0) {
      printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
      break;
    }
    if (frames > 0 && frames < (long)2*nsample/NCHAN)
      printf("Short write (expected %li, wrote %li)\n", (long)2*nsample/NCHAN, frames);

    if(cmd == 0)
      break;

    }
  snd_pcm_drain(handle);
  snd_pcm_close(handle);

  close_keyboard();
  return 0;
}
