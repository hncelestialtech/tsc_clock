#ifndef TSC_CLOCK_INCLUDE_H
#define TSC_CLOCK_INCLUDE_H
#include "tsc_config.h"

#define nano_gettime()  _nano_gettime()
#define nano_clock_init()   _nano_clock_init()
#define nano_clock_destroy() _nano_clock_destroy()

#ifdef TSC_CLOCKSOURCE
#define _nano_gettime()   _tsc_gettime()
#define _nano_clock_init()  _tsc_clock_init()
#define _nano_clock_destroy()   _tsc_clock_destroy()
#else
#include <time.h>
#define NSPERSEC 1000000000
#define _nano_gettime() {   \
    struct timespec clock;  \
    clock_gettime(CLOCK_REALTIME, &clock);  \
    clock.tv_sec * NSPERSEC + clock.tv_nsec;    \
}
#define _nano_clock_init() do {} while(0)
#define _nano_clock_destroy()   do {} while(0)
#endif // TSC_CLOCKSOURCE

#ifndef TSC_CLOCKSOURCE
#include "rdtsc_clock.h"
#endif // TSC_CLOCKSOURCE

#endif // TSC_CLOCK_INCLUDE_H