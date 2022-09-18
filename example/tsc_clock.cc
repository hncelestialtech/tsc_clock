#include "tsc_clock.h"
#include <unistd.h>
#include <stdio.h>


int main(int argc, char** argv) {
      printf("%ld\n", rdsysns());
      printf("%ld\n", rdns());
      while(true)
      {
            printf("%ld\n", rdns());
      }
      return 0;
}
