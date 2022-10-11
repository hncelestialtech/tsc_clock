#ifndef TSC_CLOCK_H
#define TSC_CLOCK_H

#include <stdint.h>
#include <time.h>
#include <semaphore.h>
#include <stdio.h>

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
#ifndef TSC_GLOBAL
    int64_t    calibrate_interval_ns;
    int64_t    base_ns_err;
#endif // TSC_GLOBAL
};

enum tsc_proc_t {
    kTSC_PRIMARY = 0,
    kTSC_SECONDARY
};

extern tsc_proc_t tsc_proc;

/**
 * @brief Initialize tsc environment
 * 
 * When the input parameter is primary, the function is responsible 
 * for initializing the tsc environment, and when the passed argument 
 * is secondary, the function is responsible for getting the tsc 
 * environment
 * @param proc_role process type which kTSC_PRIMARY or kTSC_SECONDARY can 
 *                  be passed
 * @return int 
 *  - On success, returns 0.
 *  - On failure, returns -1.
 */
int init_tsc_env(tsc_proc_t proc_role=kTSC_SECONDARY);

/**
 * @brief Clean up the tsc environment
 * 
 * @return int 
 *  - On success, returns 0.
 *  - On failure, returns errno.
 */
int destroy_tsc_env();

/**
 * @brief Gets the value of the tsc register without calibration
 * 
 * @return int64_t tsc value
 */
inline int64_t rdtsc_();
inline int64_t
#ifdef TSC_FORCE_INTRINSICS
rdtsc_()
{
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#else
rdtsc_()
{
    return __builtin_ia32_rdtsc();
}
#endif // TSC_FORCE_INTRINSICS

extern struct tsc_clock_t tsc_clock aligned_cache;

/**
 * @brief Calibrate the tsc register values with wall time
 * 
 */
void calibrate();

/**
 * @brief Check if clock calibration is required. Calibrate if required
 * 
 * @param now_tsc 
 */
inline void calibrate_check(int64_t now_tsc) {
    if (likely(now_tsc < tsc_clock.next_calibrate_tsc)) return;
    calibrate();
}

/**
 * @brief Gets the value of the tsc register with calibration
 * 
 * @return int64_t tsc value
 */
inline int64_t rdtsc()
{
    int64_t tsc = rdtsc_();
#ifdef SYNC_CALIBRATE
    calibrate_check(tsc);
#endif // SYNC_CALIBRATE
    return tsc;
}

/**
 * @brief Converts the tsc value to a timestamp
 * 
 * @param tsc tsc value
 * @return int64_t timestamp
 */
inline int64_t tsc2ns(int64_t tsc) {
    return tsc_clock.base_ns + (int64_t)((tsc - tsc_clock.base_tsc) * tsc_clock.ns_per_tsc);
}

/**
 * @brief Gets a nanosecond level timestamp from the system clock source
 * 
 * @return int64_t timestamp
 */
inline int64_t rdsysns() 
{
    struct timespec ns_timespec;
    clock_gettime(CLOCK_REALTIME, &ns_timespec);
    return ns_timespec.tv_nsec + ns_timespec.tv_sec * NSPERSEC;
}

/**
 * @brief Gets a nanosecond level timestamp from the tsc clock source
 * 
 * @return int64_t timestamp
 */
inline int64_t rdns()
{ 
    return tsc2ns(rdtsc()); 
}

#ifdef __cplusplus
}
#endif

#endif // TSC_CLOCK_H