#include "tsc_clock.h"
#include "rdtsc_clock.h"

#include <linux/limits.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#ifdef TSC_FMA && TSC_AVX512F && TSC_SSE2 && TSC_SSE
#include <immintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#endif // TSC_FMA && TSC_AVX512F && TSC_SSE2 && TSC_SSE

#ifdef USDT
#include <sys/sdt.h>
#endif // USDT


#include "rdtsc_datastruct.h"
#include "tsc_atomic.h"

#ifdef __cplusplus
extern "C" {
#endif

struct local_tsc_clocksoure {
    union {
        __uint128_t _;
        __extension__
        struct {
            double  coef;
            double offset;
        };
    };
    int64_t seq;
} __attribute__((aligned(64)));

static __thread struct local_tsc_clocksoure local_clocksource = {
    .coef = 0.0,
    .offset = 0.0,
    .seq = 0
};

static struct tsc_clocksource * global_clocksource;

static void
init_local_clocksource(void)
{
    local_clocksource.coef = global_clocksource->coef;
    local_clocksource.offset = global_clocksource->offset;
}

int
rdtsc_clocksource_attach(void)
{
    int ret;
    const char* tsc_config_path = TSC_CLOCK_PATH;
    int config_fd = open(tsc_config_path, O_RDWR, 0666);
    if (config_fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", tsc_config_path, strerror(errno));
        return errno;
    }
    // The memory allocated by mmap is aligned by page size by default to 4KB, 
    // so you can get a 64-byte aligned address without memory alignment
    global_clocksource = mmap(NULL, sizeof(struct tsc_clocksource), PROT_READ | PROT_WRITE, MAP_SHARED, config_fd, 0);
    if (global_clocksource == MAP_FAILED) {
        global_clocksource = NULL;
        fprintf(stderr, "Failed to get global clocksource: %s\n", strerror(errno));
        ret = errno;
        goto Failed;
    }
    if (global_clocksource->magic != TSC_MAGIC) {
        fprintf(stderr, "TSC clocksource is not enable\n");
        ret = -1;
        goto Failed;
    }

    init_local_clocksource();

    close(config_fd);
    return 0;
Failed:
    if (global_clocksource != NULL) {
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
        global_clocksource = NULL;
    }
    if (config_fd > 0) {
        close(config_fd);
    }
    return ret;
}

void
rdtsc_clocksource_deattach(void)
{
    if (global_clocksource != NULL) {
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
    }
}

__attribute__((noinline))
void
update_local_tscclocksource(void)
{
#if TSC_FMA && TSC_AVX512F && TSC_SSE2 && TSC_SSE
    __m128i tmp = _mm_load_epi32(global_clocksource->_);
    _mm_storeu_epi16(&local_clocksource._, tmp);
#else
    local_clocksource.coef = global_clocksource->coef;
    local_clocksource.offset = global_clocksource->offset;
#endif
    asm volatile("":::"memory");
    local_clocksource.seq = global_clocksource->seq;
#ifdef USDT
    DTRACE_PROBE2(trace_clocksource, trace_local, local_clocksource._.__.coef, local_clocksource._.__.offset);
#endif // USDT
}

int64_t
rdtsc_gettime(void)
{
    int64_t tsc;
    int64_t nanots;
retry:
    // The calculation of the conversion is performed directly speculatively, 
    // and since the probability of a collision is small, it can be returned 
    // directly most of the time

    tsc = rdtsc();

#if TSC_FMA && TSC_AVX512F && TSC_SSE2 && TSC_SSE
    // gcc 9 -O3 only impl with mulsd and addsd which can be done with fmadd
    __m128d d;
    __m128d ftsc = _mm_cvtu64_sd(d, tsc);
    __m128d coef = _mm_load_pd((double const*)&local_clocksource);
    __m128d offset = (__m128d)_mm_movehl_ps((__m128)coef, (__m128)coef);
    __m128d ts = _mm_fmadd_pd(ftsc, coef, offset);
    nanots = _mm_cvttsd_i64(ts);
#else
    nanots = local_clocksource.coef * tsc + local_clocksource.offset;
#endif // TSC_FMA && TSC_AVX512F && TSC_SSE2 && TSC_SSE
    if (__builtin_expect(!!(local_clocksource.seq != global_clocksource->seq), 0)) {
        update_local_tscclocksource();
        // The update of the global environment is much larger than the time for 
        // thread scheduling and logical calculation, so you can jump out of the
        // loop after entering the next loop without causing an infinite loop
        goto retry;
    }

    return nanots;
}

#ifdef __cplusplus
}
#endif
