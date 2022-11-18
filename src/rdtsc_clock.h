#ifndef RDTSC_CLOCK_H
#define RDTSC_CLOCK_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define _tsc_gettime()  rdtsc_gettime()
#define _tsc_clock_init()   rdtsc_clocksource_attach()
#define _tsc_clock_destroy()    rdtsc_clocksource_deattach()

static inline int64_t
rdtsc(void)
{
    union {
        int64_t tsc_64;
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
 *      Initialize the TSC clock source environment
 * @return int
 *      --0 Success
 *      --errno On error, the return value is set as errno to indicate
 *              the cause of the error
 */
int rdtsc_clocksource_attach(void);

/**
 * @brief 
 *      Obtain nanosecond-precision timestamps from the TSC clock source
 * @return int64_t
 *      timestamp 
 */
int64_t rdtsc_gettime(void);

/**
 * @brief 
 *      destroy tsc environment
 */
void rdtsc_clocksource_deattach(void);

#ifdef __cplusplus
}
#endif

#endif // RDTSC_CLOCK_H