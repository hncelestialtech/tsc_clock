#include "tsc_clock.h"
#include <unistd.h>
#include <stdio.h>


int main(int argc, char** argv) {
      init_tsc_env();
      auto t1 = rdsysns();
      auto t2 = rdns();
      return 0;
}
