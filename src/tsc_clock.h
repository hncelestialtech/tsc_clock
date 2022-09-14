#ifndef TSC_CLOCK_H
#define TSC_CLOCK_H

#include <stdint.h>
#include <time.h>

#include "tsc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NSPERSEC 1000000000

extern double ns_per_tsc_;
extern int64_t base_tsc_;
extern int64_t base_ns_;
extern int64_t calibate_interval_ns_;
extern int64_t base_ns_err_;
extern int64_t next_calibrate_tsc_;

extern void calibrate(int64_t now_tsc);

inline void calibrateCheck(int64_t now_tsc) {
    if (likely(now_tsc < next_calibrate_tsc_)) return;
    calibrate(now_tsc);
}

inline int64_t rdtsc()
{
    int64_t tsc = __builtin_ia32_rdtsc();
    calibrateCheck(tsc);
    return tsc;
}

inline int64_t tsc2ns(int64_t tsc) {
    return base_ns_ + (int64_t)((tsc - base_tsc_) * ns_per_tsc_);
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