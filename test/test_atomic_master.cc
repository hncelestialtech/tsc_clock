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
write_gclock(struct tsc_clocksource* gclock)
{
    set_cpu(1);
    sleep(5);
    struct timespec begin;
    clock_gettime(CLOCK_REALTIME, &begin);
    struct tsc_clocksource tmp;
    tmp._.__.coef = 3.14;
    tmp._.__.offset = 6.28;
    atomic_store128((uint128_atomic_t*)gclock, *(uint128_atomic_t*)&tmp);
    gclock->magic = 1;
    __asm __volatile ("lock; orl $0, (%%rsp)" ::: "memory");
    fprintf(stdout, "master timestamp %ld\n", begin.tv_sec * 1000000000 + begin.tv_nsec);
}

static void
test_cmpxhg16()
{
    const char* path = "./test.txt";
    int fd = open(path, O_RDWR|O_CREAT, 0666);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s\n", path);
        exit(-1);
    }
    ftruncate(fd, sizeof(tsc_clocksource));
    struct tsc_clocksource* global_clocksource = (struct tsc_clocksource*)mmap(NULL, sizeof(struct tsc_clocksource), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (global_clocksource == MAP_FAILED) {
        global_clocksource = NULL;
        fprintf(stderr, "Failed to get global clocksource\n");
        close(fd);
        exit(-1);
    }
    memset(global_clocksource, 0, sizeof(struct tsc_clocksource));
    close(fd);
    pid_t slave = fork();
    if (slave < 0) {
        printf("\n\n");
        printf("fork() create child process failure: %s\n",strerror(errno));
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
        exit(-1);
    }
    else if(slave == 0) {
        printf("\n");
        printf("child process PID[ %d ] start running,my parent PID is [ %d ]\n",getpid(),getppid());
        execl("./slave", NULL);
    }
    else    //if(pid > 0)
    {
        printf("\n");
        printf("parent process PID[%d] continue running,and child process PID is [ %d ]\n",getpid(),slave);
        printf("\n");
        write_gclock(global_clocksource);
        munmap(global_clocksource, sizeof(struct tsc_clocksource));
    }
}

int main()
{
    test_cmpxhg16();
    return 0;
}