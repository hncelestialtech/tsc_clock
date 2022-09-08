#include "tsc_clock.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
using namespace std;


int main(int argc, char** argv) {
//   cout << std::setprecision(15) << "init tsc_ghz: " << tn.getTscGhz() << endl;
  printf("in main ns_per_tsc %lf base_tsc %ld\n", ns_per_tsc_, base_tsc_);
  double rdns_latency;
  {
    const int N = 1000;
    int64_t tmp = 0;
    int64_t t0 = rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += rdsysns();
    }
    int64_t t1 = rdsysns();
    for (int i = 0; i < N; i++) {
      tmp += rdtsc();
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

  while (true) {
    int64_t a = rdns();
    int64_t b = rdns();
    int64_t c = rdsysns();
    int64_t d = rdns();
    int64_t b2c = c - b;
    int64_t c2d = d - c;
    int64_t err = 0;
    if (b2c < 0)
      err = -b2c;
    else if (c2d < 0)
      err = c2d;
    // calibrate_latency should not be a large value, especially not negative
    int64_t calibrate_latency = b - a - (int64_t)rdns_latency;
    int64_t rdsysns_latency = d - b - (int64_t)rdns_latency;
    cout << "calibrate_latency: " << calibrate_latency 
         << ", b2c: " << b2c << ", c2d: " << c2d << ", err: " << err
         << ", rdsysns_latency: " << rdsysns_latency << endl;
  }
      return 0;
}
