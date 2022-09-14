#ifndef TSC_COMMON_H
#define TSC_COMMON_H

#include "rte_spinlock.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#ifndef always_inline
#define always_inline __attribute__((always_inline))
#endif // always_inline
#ifndef noinlnie
#define noinline __attribute__((noinlnie))
#endif // noinline

#ifndef cold
#define cold __attribute__((cold))
#endif // cold
#ifndef hot
#define hot __attribute__((hot))
#endif // hot

#ifndef constructor
#define constructor __attribute((constructor))
#endif // constructor
#ifndef destructor
#define destructor __attribute__((destructor))
#endif // destructor

#define aligned_cache __attribute__((aligned(128)))

#define INIT_CALIBRATE_NS   20000000

#define NSPERMS 1000000
#define USPERNS 1000

#define CALIBRATE_INTERVAL_NS 1 * NSPERMS

#define NSPERSEC 1000000000

#define spinlock_t rte_spinlock_t

#ifndef MAX_PROCESS
#define MAX_PROCESS 10
#endif

#endif // TSC_COMMON_H