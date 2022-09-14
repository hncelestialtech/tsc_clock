#ifndef TSC_COMMON_H
#define TSC_COMMON_H

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

#endif // TSC_COMMON_H