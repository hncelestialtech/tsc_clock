#include "rdtscclocksource.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void cleanup(int sig __attribute__((unused)))
{
    rdtsc_clocksource_destroy();
    exit(-1);
}

int main()
{
    int ret = rdtsc_clocksource_create();
    if (ret != 0) {
        fprintf(stderr, "Failed to create clock source\n");
        exit(-1);
    }

    struct sigaction act, oldact;
    act.sa_handler = cleanup;
    sigemptyset(&act.sa_mask); 
    sigaddset(&act.sa_mask, SIGINT);
    sigaction(SIGINT, &act, &oldact);

    while(1) {
        sleep(1);
        rdtsc_clocksource_calibrate();
    }

    return 0;
}