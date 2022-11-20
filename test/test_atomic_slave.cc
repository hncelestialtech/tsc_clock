#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h> 
#include <time.h>

#include "tsc_atomic.h"
#include "rdtsc_datastruct.h"

inline int set_cpu(int i)  
{  
    cpu_set_t mask;  
    CPU_ZERO(&mask);  
  
    CPU_SET(i,&mask);  
  
    printf("thread %u, i = %d\n", pthread_self(), i);  
    if(-1 == pthread_setaffinity_np(pthread_self() ,sizeof(mask),&mask))  
    {  
        fprintf(stderr, "pthread_setaffinity_np erro\n");  
        return -1;  
    }  
    return 0;  
}

static void
test_atomic_read()
{
    set_cpu(2);
    const char* path = "./test.txt";
    int fd = open(path, O_RDWR);
    struct tsc_clocksource* gclock = (struct tsc_clocksource*)mmap(NULL, sizeof(struct tsc_clocksource), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (gclock == MAP_FAILED) {
        fprintf(stderr, "Failed to get global clocksource\n");
        close(fd);
        exit(-1);
    }
    while(gclock->magic == 0) {
        __asm__ volatile("pause":::"memory");
    }
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    auto tmp = atomic_load128((uint128_atomic_t*)gclock);
    struct tsc_clocksource localclock = *(struct tsc_clocksource*)&tmp;
    fprintf(stdout, "slave coef %lf, offset %lf\n", localclock._.__.coef, localclock._.__.offset);
    fprintf(stdout, "slave timestamp %ld\n", end.tv_sec * 1000000000 + end.tv_nsec);
}

int main()
{
    test_atomic_read();
    return 0;
}