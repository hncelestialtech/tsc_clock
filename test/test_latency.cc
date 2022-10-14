#include "tsc_clock.h"
#include <iostream>
#include <iomanip>
using namespace std;

void test_latency()
{
    double rdns_latency;
    const int N = 1000;
    int64_t tmp = 0;
    int64_t t0 = rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += rdsysns();
    }
    int64_t t1 = rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += rdtsc_();
    }
    int64_t t2 = rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += rdns();
    }
    int64_t t3 = rdsysns();
    // rdsys_latency is actually a low bound here as it's measured in a busy loop
    double rdsys_latency = (double)(t1 - t0) / (N + 1);
    double rdtsc_latency = (double)(t2 - t1 - rdsys_latency) / N;
    rdns_latency = (double)(t3 - t2 - rdsys_latency) / N;
    cout << "rdsys_latency: " << rdsys_latency << ", rdtsc_latency: " << rdtsc_latency
         << ", rdns_latency: " << rdns_latency << ", tmp: " << tmp << endl;
}

int main()
{
    init_tsc_env();
    test_latency();
    return 0;
}