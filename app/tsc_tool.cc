#define TSC_GLOBAL
#include "tsc_clock.h"

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>

const char
tsc_short_options[] = 
    "i" /* init tsc environment */
    "c" /* clean up tsc environment */
    ;

enum {
#define OPT_INIT_TSC "init"
    OPT_INIT_TSC_NUM = 'i',
#define OPT_CLEANUP_TSC "clean"
    OPT_CLEANUP_TSC_NUM = 'c'
};

extern tsc_proc_t tsc_proc;

const struct option 
tsc_long_options[] = {
    {OPT_INIT_TSC,     0, NULL, OPT_INIT_TSC_NUM},
    {OPT_CLEANUP_TSC,  0, NULL, OPT_CLEANUP_TSC_NUM}
};

void 
tsc_usage()
{
    printf("\nUsage:\n");
    printf("tsc options:\n"
        "   -i|--init       init tsc environment\n"
        "   -c|--clean      clean tsc environment\n");
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
        default:
            tsc_usage();
            break;
        }
    }
    return 0;
}