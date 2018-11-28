#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <inttypes.h>
#include "inputKey.h"

#define SMPLFREQ 48000
#define PI 3.141592
#define VOLUME 30000

int volume = VOLUME;		/* change volume */

static struct termios init_tio;
int i;
int16_t tmp;
double theta,deltatheta;


void change_buffer(int16_t *buffer,int *freq,int *nsample,char ch){
  *nsample = SMPLFREQ / *freq;
  deltatheta = PI * 2 / *nsample;
  printf("FREQ=%d deltatheta=%lf nsample=%d \n",*freq,deltatheta,*nsample);
  theta = 0;
  switch(ch){
  case 'w':
    printf("change\n");
   for(i = 0; i < *nsample*2 ; i+=2) {
     buffer[i] = sin(theta)*volume;
    theta += deltatheta;
  }
    break;

  case 'a':
    tmp = buffer[0];
    for(i = 0; i < *nsample*2 ; i+=2) {
      buffer[i] = buffer[i+2];
    }
    buffer[i] = tmp;
    printf("%"PRId16"\n",buffer[0]);
    break;
  default:
    puts("no setting");
    break;
  }

}


/*get first input state */
void init_keyboard(){
  tcgetattr(0,&init_tio);
}
void close_keyboard(){
  tcsetattr(0,TCSANOW,&init_tio);
}
/* kbhit */
int kbhit()
{
  struct termios tio;
  struct timeval tv;
  fd_set rfds;
  //set up terminal
  memcpy(&tio,&init_tio,sizeof(struct termios));
  tio.c_lflag &= ~(ICANON);
  tcsetattr(0,TCSANOW,&tio);
  //do not wait
  FD_ZERO(&rfds);
  FD_SET(0,&rfds);
  tv.tv_usec = 0;
  tv.tv_sec = 0;
  select(1,&rfds,NULL,NULL,&tv);
  //back to initial terminal mode
  tcsetattr(0,TCSANOW,&init_tio);
  return (FD_ISSET(0,&rfds)?1:0);
}

/*input key */
int getch(){
  int ch;
  struct termios tio;
  // set up terminal
  memcpy(&tio,&init_tio,sizeof(struct termios));
  tio.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO,TCSANOW,&tio);
  //input key
  read(0,&ch,1);
  //back to initial terminal mode
  tcsetattr(0,TCSANOW,&init_tio);
  return ch;
}

void InputKey(int16_t *buffer,int *freq,int *nsample,int *cmd){

  int key;
  if(kbhit()){
    /* screen clear */
    //system("cls");
    /* read key value */
    key = getch();
    printf("key=%d vol = %d\n",key,volume);

    /* control key */
    switch(key){
    case 'w':
      printf("w\n");
      *freq +=10;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 's':
      printf("s\n");
      *freq -= 10;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'a':
      printf("shift left\n");
      change_buffer(buffer,freq,nsample,'a');
      break;
    case 'e':
      printf("e: 0.5\n");
      volume = VOLUME * 0.5;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'r':
      printf("r: 0.4\n");
      volume = VOLUME * 0.4;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 't':
      printf("t: 0.3\n");
      volume = VOLUME * 0.3;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'y':
      printf("y: 0.2\n");
      volume = VOLUME * 0.2;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'u':
      printf("u: 0.1\n");
      volume = VOLUME * 0.1;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'i':
      printf("i: 0.01\n");
      volume = VOLUME * 0.01;
      change_buffer(buffer,freq,nsample,'w');
      break;
    case 'd':
      printf("d: 1.0\n");
      volume = VOLUME;
      change_buffer(buffer,freq,nsample,'w');
      break;

    case 'q':
      printf("quit!!\n");
      *cmd = 0;
      break;
    default:
      printf("error\n");
      break;
    }
  }
}



