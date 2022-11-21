#include <iostream>
#include <time.h>

#include "tsc_clock.h"

static void
test_latency()
{
  int loop_count = 10000;
  struct timespec begin{}, end{};
  clock_gettime(CLOCK_REALTIME, &begin);
  for (int i = 0; i < loop_count; ++i) {
    int64_t ts = nano_gettime();
  }
  clock_gettime(CLOCK_REALTIME, &end);
  int64_t duration = end.tv_sec * 1000000 + end.tv_nsec - (begin.tv_sec * 1000000 - end.tv_nsec);
  std::cout<<"duration "<<duration / loop_count<<std::endl;
}

int main()
{
  test_latency();
}