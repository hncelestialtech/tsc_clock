#include "tsc_clock.h"
#include <unistd.h>
#include <iostream>

void test_rdns()
{
    while(true)
    {
        usleep(1);
        // if (i % 10 == 0)
            std::cout<<"clock time "<<rdns()<<std::endl;
            // rdns();
    }
}

int main()
{
    test_rdns();
    return 0;
}