#include "tsc_clock.h"
#include "rdtsc_clock.h"

#include <linux/limits.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include "rdtsc_datastruct.h"
#include "tsc_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

struct local_tsc_clocksoure {
    double  coef;
    int64_t offset;
} __attribute__((aligned(16)));

static struct local_tsc_clocksoure local_clocksource = {
    .coef = 0.0,
    .offset = 0
};

static struct tsc_clocksource * global_clocksource;

static void
init_local_clocksource(void)
{
    local_clocksource.coef = global_clocksource->coef;
    local_clocksource.offset = global_clocksource->offset;
}

int
rdtsc_clocksource_attach(void)
{
    int ret;
    const char* tsc_config_path = TSC_CLOCK_PATH;
    int config_fd = open(tsc_config_path, O_RDWR, 0666);
    if (config_fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", tsc_config_path, strerror(errno));
        return errno;
    }
    // The memory allocated by mmap is aligned by page size by default to 4KB, 
    // so you can get a 64-byte aligned address without memory alignment
    global_clocksource = mmap(NULL, sizeof(struct tsc_clocksource), PROT_READ | PROT_WRITE, MAP_SHARED, config_fd, 0);
    if (global_clocksource == MAP_FAILED) {
        global_clocksource = NULL;
        fprintf(stderr, "Failed to get global clocksource: %s\n", strerror(errno));
        ret = errno;
        goto Failed;
    }
    if (global_clocksource->magic != TSC_MAGIC) {
        fprintf(stderr, "TSC clocksource is not enable\n");
        ret = -1;
        goto Failed;
    }

    init_local_clocksource();

    close(config_fd);
    return 0;
Failed:
    if (global_clocksource != NULL) {
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
        global_clocksource = NULL;
    }
    if (config_fd > 0) {
        close(config_fd);
    }
    return ret;
}

void
rdtsc_clocksource_deattach(void)
{
    if (global_clocksource != NULL) {
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
    }
}

__attribute__((cold, noinline))
void
update_local_tscclocksource(void)
{
    *(uint128_atomic_t*)&local_clocksource = atomic_load128((uint128_atomic_t*)global_clocksource);
}

int64_t
rdtsc_gettime(void)
{
    int64_t tsc;
retry:
    // The calculation of the conversion is performed directly speculatively, 
    // and since the probability of a collision is small, it can be returned 
    // directly most of the time
    tsc = rdtsc();
    // [TODO] Whether it can be done directly through FMA instructions
    int64_t nanosec = local_clocksource.coef * tsc + local_clocksource.offset;
    // Since the READMODIFY of the global TSC clock source is atomic, only the 
    // following scenarios can occur:
    // 1. rmw g -> load g.coef -> load g.offset
    // 2. load g.coef -> rmw g -> load g.offset
    // 3. load g.coef -> load g.offset ->rmw g
    // In the first case, the local environment does not need to be updated, 
    // and the variables required to obtain timestamps do not have transaction 
    // atomicity problems
    // In the second case, the local environment needs to be updated, because 
    // the calculation factor of the timestamp comes from the local environment,
    // so as long as the update of the local environment and the global environment 
    // is an atomic transaction, transaction atomicity can be guaranteed
    // In the third case, you only need to update the entire local environment
    if (__builtin_expect(!!(local_clocksource.coef != global_clocksource->coef 
                            || local_clocksource.offset != global_clocksource->offset), 0)) {
        update_local_tscclocksource();
        // The update of the global environment is much larger than the time for 
        // thread scheduling and logical calculation, so you can jump out of the
        // loop after entering the next loop without causing an infinite loop
        goto retry;
    }
}

#ifdef __cplusplus
}
#endif
