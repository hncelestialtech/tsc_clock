#ifndef TSC_GLOBAL
#define TSC_GLOBAL
#endif // TSC_GLOBAL
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

struct tsc_clock_t tsc_clock aligned_cache = {
    .ns_per_tsc = 0,
    .base_tsc = 0,
    .base_ns = 0,
    .next_calibrate_tsc = 0,
};

tsc_proc_t tsc_proc = kTSC_SECONDARY;

struct global_tsc_clock_t* global_tsc_clock = NULL;

static const char *default_config_dir = "/dev/shm";
static const char *global_tsc = "global_tsc";

#define TSC_FILE_FMT "%s/%s"
static int
get_global_tsc_config_path(char* __restrict__ buf, size_t buflen, const char* tsc_config_dir) 
{
    int size = snprintf(buf, buflen, TSC_FILE_FMT, default_config_dir, global_tsc);
    if (size < 0)
    {
        fprintf(stderr, "Failed to get tsc config file path\n");
        return size;
    }
    buf[size] = '\0';

    return size;
}

static void
save_calibrate_param(struct global_tsc_clock_t* global_tsc_clock, 
                    int64_t base_tsc, int64_t base_ns, int64_t sys_ns, double new_ns_per_tsc)
{
    global_tsc_clock->base_ns_err = base_ns - sys_ns;
    global_tsc_clock->next_calibrate_tsc = base_tsc + 
                (int64_t)((global_tsc_clock->calibrate_interval_ns - 1000) / new_ns_per_tsc);
    global_tsc_clock->base_tsc = base_tsc;
    global_tsc_clock->base_ns = base_ns;
    global_tsc_clock->ns_per_tsc = new_ns_per_tsc;
}

void 
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
        if (tsc[i] - tsc[i - 1] < tsc[best] - tsc[best - 1]) 
            best = i;
    }
    *tsc_out = (tsc[best] + tsc[best - 1]) >> 1;
    *ns_out = ns[best];
}

static void
init_global_tsc_clock(struct global_tsc_clock_t* global_tsc_clock)
{
    int64_t base_tsc = 0, base_ns = 0, delayed_tsc = 0, delayed_ns = 0;
    double init_ns_per_tsc;
    const int sleep_interval = 10000;

    rte_spinlock_init(&global_tsc_clock->spin_lock);
    global_tsc_clock->ns_per_tsc = 0;
    global_tsc_clock->base_tsc = 0;
    global_tsc_clock->calibrate_interval_ns = CALIBRATE_INTERVAL_NS;
    global_tsc_clock->base_ns_err = 0;
    global_tsc_clock->next_calibrate_tsc = 0;

    sync_time(&base_tsc, &base_ns);
    global_tsc_clock->base_tsc = base_tsc;
    global_tsc_clock->base_ns = base_ns;

    usleep(sleep_interval);

    sync_time(&delayed_tsc, &delayed_ns);

    init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
    save_calibrate_param(global_tsc_clock, base_tsc, base_ns, base_ns, init_ns_per_tsc);

    global_tsc_clock->magic = TSC_MAGIC;
}

static void
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

    remove(global_tsc_path);
    int global_tsc_fd = open(global_tsc_path, O_CREAT|O_EXCL|O_RDWR, 0666);
    if (global_tsc_fd < 0)
        return errno;
    
    flock(global_tsc_fd, LOCK_EX);

    ret = ftruncate(global_tsc_fd, sizeof(struct global_tsc_clock_t));
    if (ret < 0) {
        ret = errno;
        goto out;
    }
    
    global_tsc_clock = (struct global_tsc_clock_t *)mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, global_tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        ret = -1;
        fprintf(stderr, "Failed to map global tsc clock file\n");
        goto out;
    }

    init_global_tsc_clock(global_tsc_clock);

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);
out:
    flock(global_tsc_fd, LOCK_UN);
    close(global_tsc_fd);
    return ret;
}

static int
attatch_global_tsc(const char* global_tsc_path, unsigned int pathlen)
{
    int ret = 0;
    int global_tsc_fd = open(global_tsc_path, O_RDWR, 0666);
    if (global_tsc_fd < 0) {
        perror("Failed to open tsc global file");
        return global_tsc_fd;
    }

    flock(global_tsc_fd, LOCK_SH);

    global_tsc_clock = (struct global_tsc_clock_t *)mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, global_tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        fprintf(stderr, "Failed to map global tsc clock file\n");
        ret = -1;
        goto out;
    }

    while (global_tsc_clock->magic != TSC_MAGIC)
        __asm__ volatile("pause" :::"memory");
    
    init_local_tsc_clock(global_tsc_clock, &tsc_clock);
out:
    flock(global_tsc_fd, LOCK_UN);
    close(global_tsc_fd);
    return ret;
}

int
init_tsc_env(tsc_proc_t proc_role)
{
    char global_tsc_path[PATH_MAX];
    tsc_proc = proc_role;
    int ret = 0;
    int err = -1;
    int pathlen = get_global_tsc_config_path(global_tsc_path, PATH_MAX, default_config_dir);
    if (pathlen < 0)
        return -1;
    if (tsc_proc == kTSC_PRIMARY)
        err = create_global_tsc(global_tsc_path, pathlen);
    else
        err = attatch_global_tsc(global_tsc_path, pathlen);
    if (err != 0)
        perror("Failed to init env");
    return ret;
}

static int
clean_global_tsc()
{
    char global_tsc_path[PATH_MAX];
    int ret;
    ret = get_global_tsc_config_path(global_tsc_path, PATH_MAX, default_config_dir);
    if (ret < 0)
        return -1;
    ret = remove(global_tsc_path);
    if (ret < 0) {
        perror("Failed to unlink global tsc clock file");
        return errno;
    }
    return 0;
}

int
destroy_tsc_env()
{
    int ret = 0;
    if (tsc_proc == kTSC_PRIMARY)
        ret = clean_global_tsc();
    return ret;
}

static void
do_calibrate()
{
    int64_t tsc, ns;
    sync_time(&tsc, &ns);
    int64_t calulated_ns = tsc2ns(tsc);
    int64_t ns_err = calulated_ns - ns;
    int64_t expected_err_at_next_calibration =
      ns_err + (ns_err - global_tsc_clock->base_ns_err) * global_tsc_clock->calibrate_interval_ns / (ns - global_tsc_clock->base_ns + global_tsc_clock->base_ns_err);
    double new_ns_per_tsc =
      global_tsc_clock->ns_per_tsc * (1.0 - (double)expected_err_at_next_calibration / global_tsc_clock->calibrate_interval_ns);
    save_calibrate_param(global_tsc_clock, tsc, calulated_ns, ns, new_ns_per_tsc);
}

void 
calibrate()
{
    if (global_tsc_clock != NULL) {
        // try to lock spinlock
        if (rte_spinlock_trylock(&global_tsc_clock->spin_lock) != 1)
            rte_spinlock_lock(&global_tsc_clock->spin_lock);
        else {
            int64_t tsc = rdtsc_();
            if (tsc > global_tsc_clock->next_calibrate_tsc) {
                do_calibrate();
            }
        }
        init_local_tsc_clock(global_tsc_clock, &tsc_clock);
        rte_spinlock_unlock(&global_tsc_clock->spin_lock);
    }
}

#ifdef __cplusplus
}
#endif