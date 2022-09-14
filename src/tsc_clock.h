#ifndef TSC_CLOCK_H
#define TSC_CLOCK_H

#include <stdint.h>
#include <time.h>

#include "tsc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct tsc_clock_t {
    double  ns_per_tsc;
    int64_t base_tsc;
    int64_t base_ns;
    int64_t next_calibrate_tsc;
};

extern __thread struct tsc_clock_t tsc_clock aligned_cache;

extern void calibrate(int64_t now_tsc);

inline void calibrate_check(int64_t now_tsc) {
    if (likely(now_tsc < tsc_clock.next_calibrate_tsc)) return;
    calibrate(now_tsc);
}

inline int64_t rdtsc()
{
    int64_t tsc = __builtin_ia32_rdtsc();
    calibrate_check(tsc);
    return tsc;
}

inline int64_t tsc2ns(int64_t tsc) {
    return tsc_clock.base_ns + (int64_t)((tsc - tsc_clock.base_tsc) * tsc_clock.ns_per_tsc);
}

inline int64_t rdsysns() 
{
    struct timespec ns_timespec;
    clock_gettime(CLOCK_REALTIME, &ns_timespec);
    return ns_timespec.tv_nsec + ns_timespec.tv_sec * NSPERSEC;
}

inline int64_t rdns()
{ 
    return tsc2ns(rdtsc()); 
}

#ifdef __cplusplus
}
#endif

#endif // TSC_CLOCK_H