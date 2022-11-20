#include "rdtscclocksource.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    int ret = rdtsc_clocksource_create();
    if (ret < 0) {
        fprintf(stderr, "Failed to create clock source\n");
        exit(-1);
    }
    while(1) {
        sleep(5);
        rdtsc_clocksource_calibrate();
    }
    return 0;
}