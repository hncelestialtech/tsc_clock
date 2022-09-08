/*
MIT License

Copyright (c) 2022 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TSC_CLOCK_H
#define TSC_CLOCK_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

#define NSPERSEC 1000000000

extern double ns_per_tsc_;
extern int64_t base_tsc_;
extern int64_t base_ns_;
extern int64_t calibate_interval_ns_;
extern int64_t base_ns_err_;
extern int64_t next_calibrate_tsc_;

extern void calibrate(int64_t now_tsc);

inline void calibrateCheck(int64_t now_tsc) {
    if (likely(now_tsc < next_calibrate_tsc_)) return;
    calibrate(now_tsc);
}

inline int64_t rdtsc()
{
    int64_t tsc = __builtin_ia32_rdtsc();
    calibrateCheck(tsc);
    return tsc;
}

inline int64_t tsc2ns(int64_t tsc) {
    return base_ns_ + (int64_t)((tsc - base_tsc_) * ns_per_tsc_);
}

inline int64_t rdsysns() 
{
    struct timespec ns_timespec;
    clock_gettime(CLOCK_REALTIME, &ns_timespec);
    return ns_timespec.tv_nsec + ns_timespec.tv_sec * NSPERSEC;
}

inline int64_t rdns()
{ 
    return tsc2ns(rdtsc()); 
}

#ifdef __cplusplus
}
#endif

#endif // TSC_CLOCK_H