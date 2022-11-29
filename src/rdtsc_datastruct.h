#ifndef RDTSC_DATASTRUCT_H
#define RDTSC_DATASTRUCT_H

#include <stdint.h>
#include "tsc_atomic.h"

struct tsc_clocksource {
    union {
        __uint128_t _;
        __extension__
        struct {
            volatile double     coef;
            volatile double    offset;
        };
    };
    char pad[128 - sizeof(((struct tsc_clocksource*)0)->_)];
    volatile int64_t             seq;
    char pad[128 - sizeof(int64_t)];
    int64_t             base_tsc;
    int64_t             calibrate_interval;
    uint64_t            magic;
} __attribute__((aligned(64)));

#endif // RDTSC_DATASTRUCT_H