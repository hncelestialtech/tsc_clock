#ifndef TSC_COMMON_H
#define TSC_COMMON_H

#include "rte_spinlock.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#ifndef tsc_always_inline
#define tsc_always_inline __attribute__((always_inline))
#endif // tsc_always_inline
#ifndef tsc_noinline
#define tsc_noinline __attribute__((noinline))
#endif // tsc_noinline

#ifndef tsc_cold
#define tsc_cold __attribute__((cold))
#endif // tsc_cold
#ifndef tsc_hot
#define tsc_hot __attribute__((hot))
#endif // tsc_hot

#ifndef tsc_constructor
#define tsc_constructor __attribute((constructor))
#endif // tsc_constructor
#ifndef tsc_destructor
#define tsc_destructor __attribute__((destructor))
#endif // tsc_destructor

#define aligned_cache __attribute__((aligned(128)))

#define INIT_CALIBRATE_NS   20000000

#define NSPERMS 1000000
#define USPERNS 1000

#define CALIBRATE_INTERVAL_NS 1000 * NSPERMS

#define NSPERSEC 1000000000

#define spinlock_t rte_spinlock_t

#ifndef MAX_PROCESS
#define MAX_PROCESS 10
#endif

#endif // TSC_COMMON_H