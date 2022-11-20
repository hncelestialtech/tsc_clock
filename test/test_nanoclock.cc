#include "tsc_clock.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
    nano_clock_init();
    while(true) {
        auto ts = nano_gettime();
        fprintf(stdout, "ts %ld\n", ts);
        usleep(500000);
    }
}