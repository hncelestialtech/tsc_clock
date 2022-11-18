#ifndef RDTSC_DATASTRUCT_H
#define RDTSC_DATASTRUCT_H

#include <stdint.h>

struct tsc_clocksource {
    volatile double     coef;
    volatile int64_t    offset;
    uint64_t            magic;
} __attribute__((aligned(64)));

#endif // RDTSC_DATASTRUCT_H