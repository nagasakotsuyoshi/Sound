/*********************************************************************
  dumpwave.c

  UNIX:   gcc dumpwave.c -o dumpwave
  MS-DOS: gcc dumpwave.c -o dumpwave.exe

  Usage: dumpwave filename.wav
           or specify number of samples to output, e.g.,
         dumpwave -100 filename.wav (outputs 100 samples)
*********************************************************************/

/* downloaded from
 * http://oku.edu.mie-u.ac.jp/~okumura/wavefmt.html
 *
 * modified a little by A.Date for a class use. (29 Apr 2008)
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long samples_to_output = -1;

unsigned long get_ulong(FILE *f)
{
    unsigned char s[4];

    if (fread(s, 4, 1, f) != 1) {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    return s[0] + 256LU * (s[1] + 256LU * (s[2] + 256LU * s[3]));
}

unsigned get_ushort(FILE *f)
{
    unsigned char s[2];

    if (fread(s, 2, 1, f) != 1) {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    return s[0] + 256U * s[1];
}

void dumpwave(char *filename)
{
    FILE *f;
    int i, x, channels, bits;
    unsigned long len;
    long count;
    unsigned char s[10];

    if ((f = fopen(filename, "rb")) == NULL) {
        printf("Can't open %s\n", filename);
        return;
    }
    printf("# filename = '%s'\n", filename);
    if (fread(s, 4, 1, f) != 1) {
        printf("Read error\n");
        fclose(f);
        return;
    }
    if (memcmp(s, "RIFF", 4) != 0) {
        printf("Not a 'RIFF' format\n");
        fclose(f);
        return;
    }
    printf("# [RIFF] (%lu bytes)\n", get_ulong(f));
    if (fread(s, 8, 1, f) != 1) {
        printf("Read error\n");
        fclose(f);
        return;
    }
    if (memcmp(s, "WAVEfmt ", 8) != 0) {
        printf("Not a 'WAVEfmt ' format\n");
        fclose(f);
        return;
    }
    len = get_ulong(f);
    printf("# [WAVEfmt ] (%lu bytes)\n", len);
    if (len != 16) {
        printf("# Length of 'WAVEfmt ' must be 16!\n");
        return;
    }
    printf("#  Data type = %u (1 = PCM)\n", get_ushort(f));
    channels = get_ushort(f);
    printf("#  Number of channels = %u (1 = mono, 2 = stereo)\n", channels);
    printf("#  Sampling rate = %luHz\n", get_ulong(f));
    printf("#  Bytes / second = %lu\n", get_ulong(f));
    printf("#  Bytes x channels = %u\n", get_ushort(f));
    bits = get_ushort(f);
    printf("#  Bits / sample = %u\n", bits);
    while (fread(s, 4, 1, f) == 1) {
        len = get_ulong(f);
        s[4] = 0;
        printf("# [%s] (%lu bytes)\n", s, len);
        if (memcmp(s, "data", 4) == 0) break;
        for (i = 0; i < len; i++)
            printf("%02x ", fgetc(f));
        printf("\n");
    }
    for (count = 0; count != samples_to_output; count++) {
        for (i = 0; i < channels; i++) {
            if (bits <= 8) {
                if ((x = fgetc(f)) == EOF) goto loopend;
                x -= 128;
            } else {
                if (fread(s, 2, 1, f) != 1) goto loopend;
                x = (short)(s[0] + 256 * s[1]);
            }
            printf("%6d", x);
            if (i != channels - 1) printf(" ");
        }
        printf("\n");
    }
 loopend:
    fclose(f);
}

int main(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-')
            samples_to_output = atol(&argv[i][1]);
        else
            dumpwave(argv[i]);
    }
    return 0;
}
