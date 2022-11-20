#ifndef RDTSC_DATASTRUCT_H
#define RDTSC_DATASTRUCT_H

#include <stdint.h>

struct tsc_clocksource {
    union {
        __uint128_t align_var;
        struct {
            volatile double     coef;
            volatile double    offset;
        }__;
    }_;
    uint64_t            magic;
} __attribute__((aligned(64)));

#endif // RDTSC_DATASTRUCT_H