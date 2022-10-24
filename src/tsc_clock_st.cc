#undef TSC_GLOBAL
#include "tsc_clock.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct tsc_clock_t tsc_clock aligned_cache = {
    .ns_per_tsc = 0,
    .base_tsc = 0,
    .base_ns = 0,
    .next_calibrate_tsc = 0,
    .calibrate_interval_ns = CALIBRATE_INTERVAL_NS,
    .base_ns_err = 0,
};

tsc_proc_t tsc_proc = kTSC_PRIMARY;

int
init_tsc_env(tsc_proc_t proc_role)
{   return 0;   }

int 
destroy_tsc_env()
{   return 0;   }

static void 
sync_time(int64_t* tsc_out, int64_t* ns_out) 
{
    const int N = 3;
    int64_t tsc[N + 1];
    int64_t ns[N + 1];

    tsc[0] = rdtsc_();
    for (int i = 1; i <= N; i++) {
        ns[i] = rdsysns();
        tsc[i] = rdtsc_();
    }

    int j = N + 1;
    int best = 1;
    for (int i = 2; i < j; i++) {
        if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1]) best = i;
    }
    *tsc_out = (tsc[best] + tsc[best - 1]) >> 1;
    *ns_out = ns[best];
}

static void 
save_calibrate_param(int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc) 
{
    tsc_clock.base_ns_err = base_ns - sys_ns;
    tsc_clock.next_calibrate_tsc = base_tsc + (int64_t)((tsc_clock.calibrate_interval_ns - 1000) / new_ns_per_tsc);
    tsc_clock.base_tsc = base_tsc;
    tsc_clock.base_ns = base_ns;
    tsc_clock.ns_per_tsc = new_ns_per_tsc;
}

void 
calibrate() {
    int64_t tsc, ns;
    sync_time(&tsc, &ns);
    int64_t calulated_ns = tsc2ns(tsc);
    int64_t ns_err = calulated_ns - ns;
    int64_t expected_err_at_next_calibration =
      ns_err + (ns_err - tsc_clock.base_ns_err) * tsc_clock.calibrate_interval_ns / (ns - tsc_clock.base_ns + tsc_clock.base_ns_err);
    double new_ns_per_tsc =
      tsc_clock.ns_per_tsc * (1.0 - (double)expected_err_at_next_calibration / tsc_clock.calibrate_interval_ns);
    save_calibrate_param(tsc, calulated_ns, ns, new_ns_per_tsc);
}

#ifdef LINK_INIT
tsc_constructor
#endif // LINK_INT
void 
init_clock()
{
    tsc_clock.calibrate_interval_ns = CALIBRATE_INTERVAL_NS;
    int64_t base_tsc = 0, base_ns = 0;
    sync_time(&base_tsc, &base_ns);
    usleep(1000);
    int64_t delayed_tsc, delayed_ns;
    sync_time(&delayed_tsc, &delayed_ns);
    double init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
    save_calibrate_param(base_tsc, base_ns, base_ns, init_ns_per_tsc);
}

#ifdef __cplusplus
}
#endif