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
static void write_word(FILE* fp, unsigned long value, unsigned size)
{
  for ( ; size; --size, value >>= 8 ) { char v = (char)(value & 0xFF); fwrite(&v,1,1,fp); }
}
static double gettimeofday_sec(void)
{
    struct timeval tv; gettimeofday(&tv,NULL); return (tv.tv_sec + tv.tv_usec * 1e-6);
}
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
    char filename[256]; sprintf(filename,"record_%02d.wav",rch);
    
    FILE* fp = fopen(filename,"wb"); if ( fp == NULL ) return;
    
    
    unsigned int sample_rate = desire_rate;
    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
    snd_pcm_t* capture_handle = PreparePCM(rch,format,&sample_rate);
    printf("* SAMPLE RATE: %d\n",sample_rate);
    if ( capture_handle == NULL ) { fclose(fp); remove(filename); return; }
 

    // - Ready buffer
    int buffer_frames = sample_rate, length = buffer_frames * snd_pcm_format_width(format) / 8;
    char* buffer = (char*)malloc(length);
    printf("audio interface prepared: %d, length - %d\n",snd_pcm_format_width(format),length);
    
    // - [WAVE] Write the file headers
    int nSamplesPerSec = (sample_rate * 16 * 2) / 8;
    /* file wav */
    fprintf(fp,"RIFF----WAVEfmt ");     // (chunk size to be filled in later)
    write_word(fp,16,4); // no extension data
    write_word(fp,1,2);  // PCM - integer samples
    write_word(fp,1,2);  // channels
    write_word(fp,sample_rate,4);    // samples per second (Hz)
    write_word(fp,nSamplesPerSec,4); // (Sample Rate * BitsPerSample * Channels) / 8
    write_word(fp,4,2);  // data block size (size of two integer samples, one for each channel, in bytes)
    write_word(fp,16,2); // number of bits per sample (use a multiple of 8)


    
    // - [WAVE] Write the data chunk header
    size_t data_chunk_pos = ftell(fp);
    fprintf(fp,"data----"); // (chunk size to be filled in later)
    // - [ASOUND] Record and write data
    int err;
    double start = gettimeofday_sec();
    while ( bRunning == TRUE ) {
        if ( (err = snd_pcm_readi(capture_handle,buffer,buffer_frames)) != buffer_frames ) {
            printf("%d: read from audio interface failed (%s)\n",err,snd_strerror(err)); break;
        }
        fwrite(buffer,sizeof(char),length,fp);
        if ( gettimeofday_sec() - start > seconds ) break;
    }
    free(buffer);
    // - [WAVE] Fix PCM/RIFF chunk size
    size_t file_length = ftell(fp);
    /* file txt */
    fseek(fp,data_chunk_pos + 4,SEEK_SET); write_word(fp,file_length - data_chunk_pos + 8,4);
    fseek(fp,0 + 4,SEEK_SET); write_word(fp,file_length - 8,4);
    fflush(fp); fclose(fp);

    
    // - [ASOUND] close
    snd_pcm_close(capture_handle);
    printf("audio interface closed\n");
}
// example of arecord
// % arecord hw:1 (plughw:1)
// % arecord -l -> card info.
// % arecord -c 1 -f S16_LE -r 44100 sample.wav -D hw:1 (plughw:1) -> record
int main(int argc, char *argv[])
{
    struct sigaction sa; sa.sa_flags = 0; sa.sa_handler = SigIntAction;
    sigemptyset(&sa.sa_mask); sigaction(SIGINT,&sa,NULL);

    bRunning = TRUE;
    StartRecorder(0,44100,1);
    bRunning = FALSE;
    
    return 0;
}
