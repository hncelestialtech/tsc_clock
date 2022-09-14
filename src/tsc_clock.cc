#include "tsc_clock.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

double ns_per_tsc_ = 0;
int64_t base_tsc_ = 0;
int64_t base_ns_ = 0;
int64_t calibate_interval_ns_ = 0;
int64_t base_ns_err_ = 0;
int64_t next_calibrate_tsc_ = 0;

#ifndef INIT_CALIBRATE_NS
#define INIT_CALIBRATE_NS   20000000
#endif // INIT_CALIBRATE_NS

#define NSPERMS 1000000
#define USPERNS 1000

#ifndef CALIBRATE_INTERVAL_NS
#define CALIBRATE_INTERVAL_NS 1 * NSPERMS
#endif // CALIBRATE_INTERVAL_NS

inline void syncTime(int64_t* tsc_out, int64_t* ns_out) 
{
    const int N = 3;
    int64_t tsc[N + 1];
    int64_t ns[N + 1];

    tsc[0] = __builtin_ia32_rdtsc();
    for (int i = 1; i <= N; i++) {
        ns[i] = rdsysns();
        tsc[i] = __builtin_ia32_rdtsc();
    }

    int j = N + 1;
    int best = 1;
    for (int i = 2; i < j; i++) {
        if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1]) best = i;
    }
    *tsc_out = (tsc[best] + tsc[best - 1]) >> 1;
    *ns_out = ns[best];
}

void saveParam(int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc) 
{
    base_ns_err_ = base_ns - sys_ns;
    next_calibrate_tsc_ = base_tsc + (int64_t)((calibate_interval_ns_ - 1000) / new_ns_per_tsc);
    base_tsc_ = base_tsc;
    base_ns_ = base_ns;
    ns_per_tsc_ = new_ns_per_tsc;
}

noinline void calibrate(int64_t now_tsc) {
    int64_t tsc, ns;
    syncTime(&tsc, &ns);
    int64_t calulated_ns = tsc2ns(tsc);
    int64_t ns_err = calulated_ns - ns;
    int64_t expected_err_at_next_calibration =
      ns_err + (ns_err - base_ns_err_) * calibate_interval_ns_ / (ns - base_ns_ + base_ns_err_);
    double new_ns_per_tsc =
      ns_per_tsc_ * (1.0 - (double)expected_err_at_next_calibration / calibate_interval_ns_);
    saveParam(tsc, calulated_ns, ns, new_ns_per_tsc);
}

constructor void init_clock()
{
    calibate_interval_ns_ = CALIBRATE_INTERVAL_NS;
    int64_t base_tsc = 0, base_ns = 0;
    syncTime(&base_tsc, &base_ns);
    usleep(1000);
    int64_t delayed_tsc, delayed_ns;
    syncTime(&delayed_tsc, &delayed_ns);
    double init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
    saveParam(base_tsc, base_ns, base_ns, init_ns_per_tsc);
}

#ifdef __cplusplus
}
#endif