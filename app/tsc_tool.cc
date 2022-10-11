#define TSC_GLOBAL
#include "tsc_clock.h"

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>

#include <thread>

const char
tsc_short_options[] = 
    "i" /* init tsc environment */
    "c" /* clean up tsc environment */
    ;

enum {
#define OPT_INIT_TSC "init"
    OPT_INIT_TSC_NUM = 'i',
#define OPT_CLEANUP_TSC "clean"
    OPT_CLEANUP_TSC_NUM = 'c',
#define OPT_DAEMON_TSC "daemon"
    OPT_DAEMON_TSC_NUM = 'd'
};

extern tsc_proc_t tsc_proc;

const struct option 
tsc_long_options[] = {
    {OPT_INIT_TSC,     0, NULL, OPT_INIT_TSC_NUM},
    {OPT_CLEANUP_TSC,  0, NULL, OPT_CLEANUP_TSC_NUM},
    {OPT_DAEMON_TSC, 0, NULL, OPT_DAEMON_TSC_NUM}
};

void 
tsc_usage()
{
    printf("\nUsage:\n");
    printf("tsc options:\n"
        "   -i|--init       Init tsc environment\n"
        "   -c|--clean      Clean tsc environment\n"
        "   -d|--daemon     Calibrate tsc clock periodcally");
}

void
init_tsc_daemon()
{
    int ret = init_tsc_env(kTSC_SECONDARY);
    if (ret != 0) {
        fprintf(stderr, "Failed to attach tsc environment, exit\n");
        return;
    }

    std::thread tsc_daemon = std::thread([&](){
        constexpr int ns2us = 1000;
        while(true) {
            auto now = rdns();
            auto next_calibrate = tsc2ns(tsc_clock.next_calibrate_tsc);
            auto usleep_interval = (next_calibrate - now) / ns2us;
            
            usleep(usleep_interval);

            auto tsc = rdtsc_();
            calibrate_check(tsc);
        }
    });

    tsc_daemon.join();
}

int main(int argc, char** argv)
{
    int opt, ret;
    int option_index;
    tsc_proc = kTSC_PRIMARY;
    while ((opt = getopt_long(argc, argv, tsc_short_options, 
        tsc_long_options, &option_index)) != EOF) {
        if (opt == '?') {
            tsc_usage();
            ret = -1;
            break;
        }
        switch(opt) {
        case 'i':
            init_tsc_env(kTSC_PRIMARY);
            break;
        case 'c':
            destroy_tsc_env();
            break;
        case 'd':
            init_tsc_daemon();
        default:
            tsc_usage();
            break;
        }
    }
    return 0;
}