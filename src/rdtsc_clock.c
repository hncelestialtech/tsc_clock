#include "tsc_clock.h"

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tsc_clocksource {
    double coef;
    int64_t offset;
} __attribute__((aligned(16)));

int
_tsc_clock_init()
{
    const char* tsc_config_path = TSC_CLOCK_PATH;
    int config_fd = open(tsc_config_path, O_RDWR, 0666);
    if (config_fd < 0) {
        fprintf(stderr, "Failed to open %s:%s\n", tsc_config_path, );
        return -1;
    }
}

#ifdef __cplusplus
}
#endif
