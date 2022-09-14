#include "tsc_clock.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include "tsc_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TSC_MAGIC 0x19980708

struct global_tsc_clock_t {
    spinlock_t          spin_lock;     

    volatile double     ns_per_tsc;
    volatile int64_t    base_tsc;
    volatile int64_t    base_ns;
    volatile int64_t    calibrate_interval_ns;
    volatile int64_t    base_ns_err;
    volatile int64_t    next_calibrate_tsc;

    int                 magic;
} aligned_cache;


__thread struct tsc_clock_t tsc_clock;

struct global_tsc_clock_t* global_tsc_clock = NULL;

static const char *default_config_dir = "/var/run";
static const char *global_tsc = "global_tsc";
static const char *tsc_sem_name = "tsc_sem";

#define TSC_FILE_FMT "%s/%s"
static inline int
tsc_get_global_tsc_config_path(char* __restrict__ buf, size_t buflen, const char* tsc_config_dir) 
{
    int size = snprintf(buf, buflen, TSC_FILE_FMT, default_config_dir, global_tsc);
    if (size < 0)
    {
        fprintf(stderr, "failed to get tsc config file path\n");
        return size;
    }
    buf[size] = '\0';

    return size;
}

static inline void
save_calibrate_param(struct global_tsc_clock_t* global_tsc_clock, 
                    int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc)
{
    global_tsc_clock->base_ns_err = base_ns - sys_ns;
    global_tsc_clock->next_calibrate_tsc = base_tsc + (int64_t)((global_tsc_clock->calibrate_interval_ns - 1000) / new_ns_per_tsc);
    global_tsc_clock->base_tsc = base_tsc;
    global_tsc_clock->base_ns = base_ns;
    global_tsc_clock->ns_per_tsc = new_ns_per_tsc;
}

static int
notify_waiters()
{
    sem_t * tsc_sem = sem_open(tsc_sem_name, O_CREAT, 0666);
    if (tsc_sem == SEM_FAILED) {
        perror("failed to get tsc sem ");
        return -1;
    }
    for(unsigned int i = 0; i < MAX_PROCESS; ++i)
    {
        sem_post(tsc_sem);
    }
    sem_close(tsc_sem_name);
    return 0;
}

static inline void
init_global_tsc_clock(struct global_tsc_clock_t* global_tsc_clock)
{
    int base_tsc = 0, base_ns = 0, delayed_tsc = 0, delayed_ns = 0;
    double init_ns_per_tsc;
    const int sleep_interval = 1000;

    rte_spinlock_init(&global_tsc_clock->spin_lock);
    global_tsc_clock->ns_per_tsc = 0;
    global_tsc_clock->base_tsc = 0;
    global_tsc_clock->calibrate_interval_ns = 0;
    global_tsc_clock->base_ns_err = 0;
    global_tsc_clock->next_calibrate_tsc = 0;
    global_tsc_clock->magic = TSC_MAGIC;

    sync_time(&base_tsc, &base_ns);
    global_tsc_clock->base_tsc = base_tsc;
    global_tsc_clock->base_ns = base_ns;

    usleep(sleep_interval);

    sync_time(&delayed_tsc, &delayed_ns);

    double init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
    save_calibrate_param(global_tsc_clock, base_tsc, base_ns, base_ns, init_ns_per_tsc);

    notify_waiters();
}

static inline void
init_local_tsc_clock(struct global_tsc_clock_t* global_tsc_clock, struct tsc_clock_t* tsc_clock)
{
    tsc_clock->base_ns = global_tsc_clock->base_ns;
    tsc_clock->base_tsc = global_tsc_clock->base_tsc;
    tsc_clock->next_calibrate_tsc = global_tsc_clock->next_calibrate_tsc;
    tsc_clock->ns_per_tsc = global_tsc_clock->ns_per_tsc;
}

static int
create_global_tsc(const char* global_tsc_path, unsigned int pathlen)
{
    int ret = 0;
    int tsc_fd = open(global_tsc_path, O_EXCL|O_RDWR, 0666);
    if (tsc_fd < 0)
        return tsc_fd;
    
    flock(tsc_fd, LOCK_EX);

    int ret = ftruncate(tsc_fd, sizeof(struct global_tsc_clock_t));
    if (ret < 0) {
        ret = errno;
        goto out;
    }
    
    global_tsc_clock = mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        ret = -1;
        fprintf(stderr, "failed to map global tsc clock file\n");
        goto out;
    }

    init_global_tsc_clock(global_tsc_clock);

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);

out:
    flock(tsc_fd, LOCK_UN);
    close(tsc_fd);
    return ret;
}

static int
wait_global_tsc_init()
{
    sem_t * tsc_sem = sem_open(tsc_sem_name, O_CREAT, 0666);
    if (tsc_sem == SEM_FAILED) {
        perror("failed to get tsc sem ");
        return -1;
    }
    sem_wait(tsc_sem);
    sem_close(tsc_sem);
    return 0;
}

static int
attatch_global_tsc(const char* global_tsc_path, unsigned int pathlen)
{
    int ret = wait_global_tsc_init();
    if (!ret)
        return ret;

    global_tsc_clock = mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        fprintf(stderr, "failed to map global tsc clock file\n");
        return -1;
    }

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);
    return 0;
}

constructor int
init_tsc_env()
{
    char global_tsc_path[PATH_MAX];
    int ret = -1;
    int pathlen = tsc_get_global_tsc_config_path(global_tsc_path, PATH_MAX, default_config_dir);
    if (pathlen < 0)
        return -1;
    int err = create_global_tsc(global_tsc_path, pathlen);
    if(err == EEXIST)
        ret = attatch_global_tsc(global_tsc_path, pathlen);
    return ret;
}

inline void 
sync_time(int64_t* __restrict__ tsc_out, int64_t* __restrict__ ns_out) 
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

// void saveParam(int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc) 
// {
//     base_ns_err_ = base_ns - sys_ns;
//     next_calibrate_tsc_ = base_tsc + (int64_t)((calibate_interval_ns_ - 1000) / new_ns_per_tsc);
//     base_tsc_ = base_tsc;
//     base_ns_ = base_ns;
//     ns_per_tsc_ = new_ns_per_tsc;
// }

noinline void calibrate(int64_t now_tsc) {
    int64_t tsc, ns;
    sync_time(&tsc, &ns);
    int64_t calulated_ns = tsc2ns(tsc);
    int64_t ns_err = calulated_ns - ns;
    int64_t expected_err_at_next_calibration =
      ns_err + (ns_err - base_ns_err_) * calibate_interval_ns_ / (ns - base_ns_ + base_ns_err_);
    double new_ns_per_tsc =
      ns_per_tsc_ * (1.0 - (double)expected_err_at_next_calibration / calibate_interval_ns_);
    saveParam(tsc, calulated_ns, ns, new_ns_per_tsc);
}

#ifdef __cplusplus
}
#endif