#include "tsc_clock.h"
#include <unistd.h>
#include <iostream>

void test_rdns()
{
    for(int i = 0; i < 10000; ++i)
    {
        usleep(1);
        if (i % 10 == 0)
            std::cout<<"clock time "<<rdns()<<std::endl;
    }
}

int main()
{
    pid_t fpid;
    fpid = fork();
    if (fpid < 0)
        fprintf(stderr, "error in fork\n");
    else if (fpid == 0) {
        test_rdns();
    }   
    else {
        test_rdns();
    }
    return 0;
}