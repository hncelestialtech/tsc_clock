#ifndef RDTSC_CLOCK_H
#define RDTSC_CLOCK_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t
rdtsc()
{
    union {
        uint64_t tsc_64;
        __extension__
        struct {
            uint32_t lo_32;
            uint32_t hi_32;
        };
    } tsc;
    // Both of RSTSC & RDTSCP are not serializing instructions.
	// It does not necessarily wait until all previous instructions
	// have been executed before reading the counter.
#ifdef RDTSC_PRECISE
    tsc_lfence();
#endif // RDTSC_PRECISE
    
#ifdef RDTSCP
    asm volatile("rdtscp" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
#else
    asm volatile("rdtsc" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
#endif // RDTSCP
    return tsc.tsc_64;
}

/**
 * @brief 
 * 
 * @return int 
 */
int _tsc_clock_init();

/**
 * @brief 
 * 
 * @return uint64_t 
 */
uint64_t _tsc_gettime();


#ifdef __cplusplus
}
#endif

#endif // RDTSC_CLOCK_H