#ifndef TSC_ATOMIC_H
#define TSC_ATOMIC_H
#include <stdint.h>

#define tsc_lfence() asm volatile("lfence":::"memory")

struct _uint128_atomic_t {
    volatile uint64_t low;
    volatile uint64_t high;
} __attribute__((aligned(16)));
typedef struct _uint128_atomic_t uint128_atomic_t;


static inline int
cmpexch_weak_relaxed(
    uint128_atomic_t *atomic,
    uint128_atomic_t *expected,
    uint128_atomic_t desired)
{
    int matched;
    uint128_atomic_t e = *expected;
    asm volatile("lock cmpxchg16b %1"
                 : "=@ccz"(matched), "+m"(*atomic), "+a"(e.low), "+d"(e.high)
                 : "b"(desired.low), "c"(desired.high)
                 : "cc");
    if (!matched)
        *expected = e;
    return matched;
}


static inline uint128_atomic_t
atomic_load128(uint128_atomic_t const *atomic)
{
    uint128_atomic_t ret = {0, 0};
    asm volatile("lock cmpxchg16b %1"
                 : "+A"(ret)
                 : "m"(*atomic), "b"(0), "c"(0)
                 : "cc");
    return ret;
}


static inline void
atomic_store128(uint128_atomic_t *atomic, uint128_atomic_t val)
{
    uint128_atomic_t old = *atomic;
    while (!cmpexch_weak_relaxed(atomic, &old, val))
        ;
}

#endif // TSC_ATOMIC_H