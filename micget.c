#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
//----------------------------------------------------------------------------------------------------
enum BOOL{ TRUE = 1, FALSE = 0 };
static enum BOOL bRunning = FALSE;
//----------------------------------------------------------------------------------------------------

static void SigIntAction(int s)
{
    bRunning = FALSE;
}
//----------------------------------------------------------------------------------------------------
static snd_pcm_t* PreparePCM(int hwid, snd_pcm_format_t format, unsigned int* rate)
{
    int err;
    snd_pcm_t* capture_handle = NULL;
    snd_pcm_hw_params_t* hw_params;
    char input[32]; sprintf(input,"plughw:%d",hwid);//hw:1
    
    if ( (err = snd_pcm_open(&capture_handle,input,SND_PCM_STREAM_CAPTURE,0)) < 0 ) {
        printf("error: open audio device %s (%s)\n",input,snd_strerror(err)); return NULL;
    }
    printf("audio interface opened\n");
    if ( (err = snd_pcm_hw_params_malloc(&hw_params)) < 0 ) {
        printf("error: allocate hardware with parameter (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params allocated\n");
    if ( (err = snd_pcm_hw_params_any(capture_handle,hw_params)) < 0 ) {
        printf("error: initialize hardware parameter structure (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params initialized\n");
    if ( (err = snd_pcm_hw_params_set_access(capture_handle,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 ) {
        printf("error: set access type (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params access setted\n");
    if ( (err = snd_pcm_hw_params_set_format(capture_handle,hw_params,format)) < 0 ) {
        printf("error: set sample format (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    printf("hw_params format setted\n");
    if ( (err = snd_pcm_hw_params_set_rate_near(capture_handle,hw_params,rate,0)) < 0 ) {
        printf("error: set sample rate (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    snd_pcm_hw_params_get_rate(hw_params,rate,NULL);
    
    printf("hw_params rate setted\n");
    if ( (err = snd_pcm_hw_params_set_channels(capture_handle,hw_params,1)) < 0 ) {
        printf("error: set channel count (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }

    printf("hw_params channels setted\n");
    if ( (err = snd_pcm_hw_params(capture_handle,hw_params)) < 0 ) {
        printf("error: set parameters (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    snd_pcm_hw_params_free(hw_params);
    
    if ( (err = snd_pcm_prepare(capture_handle)) < 0 ) {
        printf("error: prepare audio interface for use (%s)\n",snd_strerror(err));
        snd_pcm_close(capture_handle); return NULL;
    }
    return capture_handle;
}
static void StartRecorder(int rch, int desire_rate, int seconds)
{
  unsigned int sample_rate = desire_rate;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  snd_pcm_t* capture_handle = PreparePCM(rch,format,&sample_rate);
  // - Ready buffer

    int buffer_frames = sample_rate, length = buffer_frames * snd_pcm_format_width(format) / 8;
    char* buffer = (char*)malloc(length);
    printf("audio interface prepared: %d, length - %d\n",snd_pcm_format_width(format),length);
    
    // - [WAVE] Write the file headers
    int nSamplesPerSec = (sample_rate * 16 * 2) / 8;

    // - [ASOUND] Record and write data
#define ABS(x) ((x < 0) ? -x : x)
    int err;
    int i, j, k;
    short *sp;
    int cnt=0;
    while ( bRunning == TRUE ) {
        if ( (err = snd_pcm_readi(capture_handle,buffer,buffer_frames)) != buffer_frames ) {
            printf("%d: read from audio interface failed (%s)\n",err,snd_strerror(err)); break;
        }
	k = length / 2; /* 16bit=2byte */
	sp = buffer;
	j = 0;
	
	for (i = 0; i < k; i += 1) {
	  j += ABS(sp[i]);

	}
	printf("%d\n", j / (k / 1));
	if(cnt==10){
	  break;
	  cnt = 0;
	}
	cnt++;
    }
    free(buffer);
    // - [WAVE] Fix PCM/RIFF chunk size
    
    // - [ASOUND] close
    snd_pcm_close(capture_handle);
    printf("audio interface closed\n");

}

void record(){
    struct sigaction sa; sa.sa_flags = 0; sa.sa_handler = SigIntAction;
    sigemptyset(&sa.sa_mask); sigaction(SIGINT,&sa,NULL);

    bRunning = TRUE;
    StartRecorder(0,48000,1);
    bRunning = FALSE;

  
}


// example of arecord
// % arecord hw:1 (plughw:1)
// % arecord -l -> card info.
// % arecord -c 1 -f S16_LE -r 44100 sample.wav -D hw:1 (plughw:1) -> record
/*int main(int argc, char *argv[])
{
    struct sigaction sa; sa.sa_flags = 0; sa.sa_handler = SigIntAction;
    sigemptyset(&sa.sa_mask); sigaction(SIGINT,&sa,NULL);

    bRunning = TRUE;
    StartRecorder(0,48000,1);
    bRunning = FALSE;
    
    return 0;
}

*/
