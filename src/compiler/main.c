#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "tsc_cpuid.h"
#include "tsc_cpuflags.h"

struct tsc_config {
    char dir[100];              /**< Header file output path */
    int  use_rdtsc;             /**< Whether to use a TSC clock */
    int  use_rdtscp;            /**< Whether to use rdtscp instrument */
    int  use_sysclock;          /**< Whether to use system clock*/
    int  use_precise_clock;     /**< Whether to consider out-of-order when using tsc directives */
};

const char
tsc_short_options[] =
    "h"  // help
    "o:" // output path
    "c:" // clock
    "p"  // precise
    ;

enum {
#define OPT_HELP            "help"
    OPT_HELP_NUM        =   'h',
#define OPT_TSC_OUT         "out"
    OPT_TSC_OUT_NUM     =   'o',
#define OPT_TSC_CLOCK       "clock"
    OPT_TSC_CLOCK_NUM   =   'c',
#define OPT_TSC_PRECISE     "precise"
    OPT_TSC_PRECISE_NUM =   'p'
};

const struct option
tsc_long_options[] = {
    {OPT_HELP, 0, NULL, OPT_HELP_NUM},
    {OPT_TSC_OUT, 1, NULL, OPT_TSC_OUT_NUM},
    {OPT_TSC_CLOCK, 1, NULL, OPT_TSC_CLOCK_NUM},
    {0, 0, NULL, 0}
};

struct tsc_config default_config = {
    .dir = "./",
    .use_rdtsc = 1,
    .use_rdtscp = 0,
    .use_sysclock = 0,
    .use_precise_clock = 0
};

static void
usage()
{
    printf("\nUsage:");
    printf("[options]\n\n"
           "    -o, --"OPT_TSC_OUT" Configure the file generation classpath\n"
           "    -c, --"OPT_TSC_CLOCK" Clocksource which can chose rdtsc rdtscp sys\n"
           "    -p, --"OPT_TSC_PRECISE" Prevent out of order.Valid only if a TSC or TSCP clock source is selected\n"
           "    -h, --help  This help\n");
}

static void
parse_clock_source(char* const arg)
{
    if (strcmp(arg, "rdtsc") == 0) {
        default_config.use_rdtsc = 1;
        return;
    }
    if (strcmp(arg, "rdtscp") == 0) {
        default_config.use_rdtscp = 1;
        return;
    }
    if (strcmp(arg, "sys") == 0) {
        default_config.use_sysclock = 1;
        return;
    }
    usage();
    return;
}

static int
parse_args(int argc, char** argv)
{
    int opt, ret;
    char **argvopt;
	int option_index;
	char *prgname = argv[0];
	const int old_optind = optind;
	const int old_optopt = optopt;
	char * const old_optarg = optarg;

	argvopt = argv;
    optind = 1;
    
    while ((opt = getopt_long(argc, argvopt, tsc_short_options, 
                    tsc_long_options, &option_index)) != EOF) {
        if (opt == '?') {
            usage();
            return -1;
        }
        switch (opt) {
        case OPT_TSC_OUT_NUM:
            memcpy(default_config.dir, optarg, strlen(optarg));
            default_config.dir[ret] = '\0';
            break;
        case OPT_TSC_PRECISE_NUM:
            default_config.use_precise_clock = 1;
            break;
        case OPT_TSC_CLOCK_NUM:
            parse_clock_source(optarg);
            break;
        case OPT_HELP_NUM:
            usage();
            break;
        }
    }
    return 0;
}

static void
generate_header_guard(int start, int fd)
{
    if (start) {
        dprintf(fd, "#ifndef TSC_CONFIG_H\n#define TSC_CONFIG_H\n");
    }
    else {
        dprintf(fd, "#endif // TSC_CONFIG_H\n");
    }
}

static void
generate_common_config(int fd)
{
    dprintf(fd, "#define TSC_CLOCK_PATH \"/var/run/.tsc_clocksource\"\n");
    dprintf(fd, "#define TSC_MAGIC  0x19980708\n");
    dprintf(fd, "#define CALIBRATE_SAMPLE   128\n");
    dprintf(fd, "#define SAMPLEDURATION 16 * 1000000\n");
    dprintf(fd, "#define GET_CLOSEST_TSC_RETRIES   256\n");
    if (cpu_get_flag_enabled(CPUFLAG_AVX512F) == 1 && cpu_get_flag_enabled(CPUFLAG_FMA) == 1) {
        dprintf(fd, "#define TSC_AVX512F\n#define TSC_FMA\n");
    }
    if (cpu_get_flag_enabled(CPUFLAG_SSE) == 1) {
        dprintf(fd, "#define TSC_SSE\n");
    }
    if (cpu_get_flag_enabled(CPUFLAG_SSE2) == 1) {
        dprintf(fd, "#define TSC_SSE2\n");
    }
}

static int
cmpxchg16_is_support()
{
    return cpu_get_flag_enabled(CPUFLAG_CMPXCHG16B) == 1;
}

static int
tsc_is_support()
{
    return cpu_get_flag_enabled(CPUFLAG_TSC) == 1 && cpu_get_flag_enabled(CPUFLAG_INVTSC);
}

static int
rdtscp_is_support()
{
    return cpu_get_flag_enabled(CPUFLAG_RDTSCP) == 1;
}

static void
generate_clocksource_config(int fd)
{
    if (default_config.use_sysclock == 1)
        return;
    if (cmpxchg16_is_support() == -1) {
        fprintf(stderr, "Does not support cmpxchg16. Failed to init tsc environment!\n");
        return;
    }
    if (tsc_is_support() == -1) {
        fprintf(stderr, "Does not support rdtsc!\n");
        return;
    }
    dprintf(fd, "#define TSC_CLOCKSOURCE\n");
    if (default_config.use_rdtscp == 1) {
        if (rdtscp_is_support() == -1) {
            fprintf(stderr, "Does not support rdtscp!\n");
        }
        dprintf(fd, "#define RDTSCP\n");
    }
    if (default_config.use_precise_clock == 1)
        dprintf(fd, "#define RDTSC_PRECISE\n");
}

static void
generate_config(int fd) 
{
    generate_common_config(fd);
    generate_clocksource_config(fd);
}

#define DEFAULT_CONFIG_PATH "%s/tsc_config.h"

int main(int argc, char** argv)
{
    int ret = parse_args(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "Failed to parse args");
        exit(-1);
    }

    char config_header_path[100];
    snprintf(config_header_path, 100, DEFAULT_CONFIG_PATH, default_config.dir);
    int config_fd = open(config_header_path, O_CREAT|O_RDWR, 0666);
    if (config_fd < 0) {
        fprintf(stderr, "Failed to open config path %s: %s\n", config_header_path, strerror(errno));
        exit(-1);
    }
    generate_header_guard(1, config_fd);

    generate_config(config_fd);

    generate_header_guard(0, config_fd);

    close(config_fd);
    return 0;
}