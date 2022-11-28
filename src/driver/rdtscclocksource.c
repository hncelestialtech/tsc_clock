#include "rdtscclocksource.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "tsc_config.h"
#include "rdtsc_datastruct.h"
#include "tsc_atomic.h"

#ifdef USDT
#include <sys/sdt.h>
#endif // USDT

static struct tsc_clocksource * tsc_clocksource;

static int
rdtsc_clocksource_init(void)
{
    return rdtsc_clocksource_calibrate();
}

int
rdtsc_clocksource_create(void)
{
    int ret;
    const char* tsc_config_path = TSC_CLOCK_PATH;
    int config_fd = open(tsc_config_path, O_RDWR|O_CREAT, 0666);
    if (config_fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", tsc_config_path, strerror(errno));
        return errno;
    }
    ftruncate(config_fd, sizeof(struct tsc_clocksource));
    // The memory allocated by mmap is aligned by page size by default to 4KB, 
    // so you can get a 64-byte aligned address without memory alignment
    tsc_clocksource = mmap(NULL, sizeof(struct tsc_clocksource), PROT_READ | PROT_WRITE, MAP_SHARED, config_fd, 0);
    memset(tsc_clocksource, 0, sizeof(struct tsc_clocksource));
    if (tsc_clocksource == MAP_FAILED) {
        tsc_clocksource = NULL;
        fprintf(stderr, "Failed to get global clocksource: %s\n", strerror(errno));
        ret = errno;
        goto Failed;
    }

    if (rdtsc_clocksource_init() != 0) {
        goto Failed;
    }

    tsc_full_barrier();
    tsc_clocksource->magic = TSC_MAGIC;
    tsc_full_barrier();
    close(config_fd);
    return 0;
Failed:
    if (tsc_clocksource != NULL) {
        munmap(tsc_clocksource, sizeof(struct tsc_clocksource));
        tsc_clocksource = NULL;
    }
    if (config_fd > 0) {
        close(config_fd);
    }
    return ret;
}

int
rdtsc_clocksource_destroy(void)
{
    tsc_clocksource->magic = 0;
    if (tsc_clocksource != NULL) {
        munmap(tsc_clocksource, sizeof(struct tsc_clocksource));
    }
    return 0;
}

static int64_t 
_rdtsc(void)
{
    union {
        int64_t tsc_64;
        __extension__
        struct {
            uint32_t lo_32;
            uint32_t hi_32;
        };
    } tsc;
    // There is no worry about out of order execution, just for get the gap between tsc and sysclock
    asm volatile("rdtsc" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
}

/**
 * @brief Get the closest tsc sys object
 *      Tries to get the closest tsc register value nearby the system clock in a loop.
 *      256 is enough for finding the lowest sys clock cost in most cases.
 *      Although clock_gettime is using VDSO to get time, but it's unstable,
 *      sometimes it will take more than 1000ns, we have to use a big loop(e.g. 256)
 *      to get the "real" clock.And it won't take a long time to finish calibrating job, 
 *      only about 20µs.
 * @param n 
 *      Loop count
 * @param tsc_time
 *      Pointer to closest tsc timeclock
 * @param sys_time
 *      Pointer to closest sys timeclock 
 */
static void
get_closest_tsc_sys(int n, int64_t* tsc_time, int64_t* sys_time)
{
    // [tsc_clock, wc, tsc_clock, wc, ..., tsc_clock]
    int64_t* timeline = malloc(sizeof(int64_t) * (2 * n + 1));
    
    timeline[0] = _rdtsc();
    
    for (int i = 1; i < 2 * n; i += 2) {
        // CLOCK_REALTIME clock source can be calibrated by NTP
        struct timespec realtime_clock; 
        clock_gettime(CLOCK_REALTIME, &realtime_clock);
        timeline[i] = realtime_clock.tv_nsec + realtime_clock.tv_sec * 1000000000;
        timeline[i + 1] = _rdtsc();
    }

    int64_t mindelta = UINT64_MAX;
    int minindex = 1;
    for (int i = 1; i < 2 * n; i += 2) {
        int64_t last = timeline[i];
        for (int j = i + 2; j < 2 * n; j += 2) {
            if (timeline[j] != last) {
                int64_t mid = (i + j - 2)>> 1;
                if (mid & 1 == 0) {
                    mid++;
                }

                int64_t delta = llabs(timeline[mid + 1] - timeline[mid - 1]);
                
                if (delta < mindelta) {
                    mindelta = delta;
                    minindex = mid;
                }

                i = j;
                last = timeline[j];
            }
        }
    }

    *tsc_time = (timeline[minindex + 1] + timeline[minindex - 1]) >> 1;
    *sys_time = timeline[minindex];
    
    free(timeline);
    return;
}

// simpleLinearRegression without intercept:
// α = ∑𝑥𝑖𝑦𝑖 / ∑𝑥𝑖^2.
static void
simple_linear_regression(double* tscs, double* syss, size_t len, double* coef, double* offset)
{
    double tsc_mean, sys_mean, denominator, numerator;
    for (int i = 0; i < len; ++i) {
        tsc_mean += tscs[i];
        sys_mean += syss[i];
    }

    tsc_mean /= len;
    sys_mean /= len;

    for (int i = 0; i < len; ++i) {
        numerator += (tscs[i] - tsc_mean) * (syss[i] - sys_mean);
        denominator += (tscs[i] - tsc_mean) * (tscs[i] - tsc_mean);
    }

    *coef = numerator / denominator;
    *offset = (sys_mean - *coef * tsc_mean);
}

static inline void
adjust_coef(struct tsc_clocksource* clocksource, int64_t base_tsc, double coef, double offset)
{
    if (__builtin_expect(!!(clocksource->base_tsc != 0), 1)) {
        int64_t ns_now = clocksource->_.__.coef * base_tsc + clocksource->_.__.offset;
        int64_t next_calibrate_ns = ns_now + clocksource->calibrate_interval;
        int64_t next_calibrate_tsc = (next_calibrate_ns - clocksource->_.__.offset) / coef;
        double coef = clocksource->calibrate_interval / (next_calibrate_tsc - clocksource->base_tsc);
        offset = next_calibrate_ns - next_calibrate_tsc * coef;
    }
    clocksource->base_tsc = base_tsc;
    clocksource->_.__.coef = coef;
    clocksource->_.__.offset = offset;
}

int 
rdtsc_clocksource_calibrate(void)
{
    struct tsc_clocksource tmp;
    double* tscs = malloc(sizeof(double) * CALIBRATE_SAMPLE * 2);
    double* syss = malloc(sizeof(double) * CALIBRATE_SAMPLE * 2);
    double coef;
    double offset;
    int64_t tsc_now = rdtsc();

    for (int i = 0; i < CALIBRATE_SAMPLE; ++i) {
        int64_t tsc0, sys0, tsc1, sys1;
        get_closest_tsc_sys(GET_CLOSEST_TSC_RETRIES, &tsc0, &sys0);
        usleep(SAMPLEDURATION);
        get_closest_tsc_sys(GET_CLOSEST_TSC_RETRIES, &tsc1, &sys1);

        tscs[i * 2] = (double)tsc0;
        tscs[i * 2 + 1] = (double)tsc1;

        syss[i * 2] = (double)sys0;
        syss[i * 2 + 1] = (double)sys1;
    }

    simple_linear_regression(tscs, syss, CALIBRATE_SAMPLE << 1, &coef, &offset);

    adjust_coef(tsc_clocksource, tsc_now, coef, offset);

    fprintf(stdout, "update coef %lf, offset %lf\n", tmp._.__.coef, tmp._.__.offset);

    atomic_store128((uint128_atomic_t*)tsc_clocksource, *(uint128_atomic_t*)&tmp);

#ifdef USDT
    DTRACE_PROBE2(trace_clocksource, trace_global, tsc_clocksource->_.__.coef, tsc_clocksource->_.__.offset);
#endif // USDT

    free(tscs);
    free(syss);

    return 0;
}