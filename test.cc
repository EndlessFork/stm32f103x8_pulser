

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define _CONFIG_H_
#define MIN_REPEAT 256
#include "pattern.c" // !
#include "random.c" // !

// gcc -o test test.cc && ./test


int main(int argc, char **argv) {
   int pattern;
   uint32_t period, bufsize;
   uint8_t *buffer;
   char filename[30];
   FILE *fp;

   buffer = (uint8_t*) malloc(1<<25);
   for (pattern=0; pattern < get_max_pattern_number(); pattern++) {
       printf("test pattern %d\n", pattern);
       select_pattern(pattern);
       period = fill_buffer(NULL, 0, 0);
       bufsize = period;

       while (bufsize < 1048576*16)
           bufsize += period;
       
       //buffer = (uint8_t*) malloc(bufsize*2);
       fill_buffer(buffer, bufsize, 0);
       
       // do something else
       sprintf(filename, "pattern_%d.dat", pattern);
       fp = fopen(filename, "w");
       fwrite(buffer, bufsize, 1, fp);
       fclose(fp);

   }
       free(buffer);
}
