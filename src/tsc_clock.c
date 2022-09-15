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


__thread struct tsc_clock_t tsc_clock aligned_cache = {
    .base_ns = 0,
    .base_tsc = 0,
    .global_tsc_fd = -1,
    .next_calibrate_tsc = 0,
    .tsc_sem = NULL
};

struct global_tsc_clock_t* global_tsc_clock = NULL;

static const char *default_config_dir = "/var/run";
static const char *global_tsc = "global_tsc";
static const char *tsc_sem_name = "tsc_sem";

#define TSC_FILE_FMT "%s/%s"
static inline int
get_global_tsc_config_path(char* __restrict__ buf, size_t buflen, const char* tsc_config_dir) 
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
    tsc_clock.tsc_sem = sem_open(tsc_sem_name, O_CREAT, 0666);
    if (tsc_clock.tsc_sem == SEM_FAILED) {
        perror("failed to get tsc sem ");
        return -1;
    }
    for(unsigned int i = 0; i < MAX_PROCESS; ++i)
    {
        sem_post(tsc_clock.tsc_sem);
    }
    return 0;
}

inline void 
sync_time(int64_t* __restrict__ tsc_out, int64_t* __restrict__ ns_out) 
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

static inline void
init_global_tsc_clock(struct global_tsc_clock_t* global_tsc_clock)
{
    int64_t base_tsc = 0, base_ns = 0, delayed_tsc = 0, delayed_ns = 0;
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

    init_ns_per_tsc = (double)(delayed_ns - base_ns) / (delayed_tsc - base_tsc);
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
    tsc_clock.global_tsc_fd = open(global_tsc_path, O_EXCL|O_RDWR, 0666);
    if (tsc_clock.global_tsc_fd < 0)
        return tsc_clock.global_tsc_fd;
    
    flock(tsc_clock.global_tsc_fd, LOCK_EX);

    ret = ftruncate(tsc_clock.global_tsc_fd, sizeof(struct global_tsc_clock_t));
    if (ret < 0) {
        ret = errno;
        goto out;
    }
    
    global_tsc_clock = mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, tsc_clock.global_tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        ret = -1;
        fprintf(stderr, "failed to map global tsc clock file\n");
        goto out;
    }

    init_global_tsc_clock(global_tsc_clock);

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);

out:
    flock(tsc_clock.global_tsc_fd, LOCK_UN);
    return ret;
}

static int
wait_global_tsc_init()
{
    tsc_clock.tsc_sem = sem_open(tsc_sem_name, O_CREAT, 0666);
    if (tsc_clock.tsc_sem == SEM_FAILED) {
        perror("failed to get tsc sem ");
        return -1;
    }
    sem_wait(tsc_clock.tsc_sem);
    return 0;
}

static int
attatch_global_tsc(const char* global_tsc_path, unsigned int pathlen)
{
    int ret = wait_global_tsc_init();
    if (!ret)
        return ret;

    tsc_clock.global_tsc_fd = open(global_tsc_path, O_RDWR, 0666);
    if (tsc_clock.global_tsc_fd < 0)
        return tsc_clock.global_tsc_fd;

    flock(tsc_clock.global_tsc_fd, LOCK_SH);

    global_tsc_clock = mmap(NULL, sizeof(struct global_tsc_clock_t), PROT_READ | PROT_WRITE, MAP_SHARED, tsc_clock.global_tsc_fd, 0);
    if (global_tsc_clock == MAP_FAILED) {
        fprintf(stderr, "failed to map global tsc clock file\n");
        ret = -1;
        goto out;
    }

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);
out:
    flock(tsc_clock.global_tsc_fd, LOCK_UN);
    return ret;
}

constructor int
init_tsc_env()
{
    char global_tsc_path[PATH_MAX];
    int ret = -1;
    int pathlen = get_global_tsc_config_path(global_tsc_path, PATH_MAX, default_config_dir);
    if (pathlen < 0)
        return -1;
    int err = create_global_tsc(global_tsc_path, pathlen);
    if(err == EEXIST)
        ret = attatch_global_tsc(global_tsc_path, pathlen);
    return ret;
}

static int
clean_global_tsc()
{
    char global_tsc_path[PATH_MAX];
    int ret;
    int pathlen;
    if (tsc_clock.global_tsc_fd < 0) {
        fprintf(stderr, "failed to close global tsc file\n");
        return -1;
    }
    close(tsc_clock.global_tsc_fd);
    ret = get_global_tsc_config_path(global_tsc_path, PATH_MAX, default_config_dir);
    if (ret < 0)
        return -1;
    ret = unlink(global_tsc_path);
    if (ret < 0) {
        perror("failed to unlink global tsc clock file\n");
        return ret;
    }
    return 0;
}

static int
clean_tsc_sem()
{
    if (tsc_clock.tsc_sem == NULL)
        return 0;
    int ret = sem_close(tsc_clock.tsc_sem);
    if (ret != 0) {
        perror("failed to close tsc sem");
        return ret;
    }
    ret = sem_unlink(tsc_sem_name);
    if (ret != 0) {
        perror("failed to unlink tsc sem\n");
        return ret;
    }
    return 0;
}

destructor int
destroy_tsc_env()
{
    int ret;
    ret = clean_global_tsc();
    ret |= clean_tsc_sem();
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

noinline void 
calibrate()
{
    // try to lock spinlock
    if (rte_spinlock_trylock(&global_tsc_clock->spin_lock) != 1) {
        while (rte_spinlock_is_locked(&global_tsc_clock->spin_lock)) 
            asm __volatile__("pause");
    }
    else {
        do_calibrate();
        rte_spinlock_unlock(&global_tsc_clock->spin_lock);
    }

    init_local_tsc_clock(global_tsc_clock, &tsc_clock);
}

#ifdef __cplusplus
}
#endif