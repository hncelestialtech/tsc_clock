#ifndef TSC_CONFIG_H
#define TSC_CONFIG_H
#define TSC_CLOCK_PATH "/dev/shm/.tsc_clocksource"
#define TSC_MAGIC  0x19980708
#define CALIBRATE_SAMPLE   128
#define SAMPLEDURATION 16 * 1000
#define GET_CLOSEST_TSC_RETRIES   256
#define TSC_CLOCKSOURCE
// #define TSC_AVX512F
// #define TSC_FMA
// #define TSC_SSE
#endif // TSC_CONFIG_H
