#ifndef RDTSC_DATASTRUCT_H
#define RDTSC_DATASTRUCT_H

#include <stdint.h>
#include "tsc_atomic.h"

struct tsc_clocksource {
    union {
        __uint128_t align_var;
        struct {
            volatile double     coef;
            volatile double    offset;
        }__;
    }_;
    char pad[TSC_SMP_ALIGN - sizeof(tsc_clocksource._)];
    int64_t             base_tsc;
    int64_t             calibrate_interval;
    uint64_t            magic;
} __attribute__((aligned(64)));

#endif // RDTSC_DATASTRUCT_H