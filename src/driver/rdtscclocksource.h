#ifndef TSCCLOCKSOURCE_INCLUDE_H
#define TSCCLOCKSOURCE_INCLUDE_H

/**
 * @brief 
 *      Create and initialize tsc clocksource environment
 * @return int
 *      0 --Success
 *      errno --On failed, ret is set as errno value
 */
int rdtsc_clocksource_create(void);

/**
 * @brief 
 *      Calibrate the TSC and system clock sources
 *      It's a good practice that run Calibrate periodically 
 *      (e.g., 5 min is a good start because the auto NTP adjust is always every 11 min).
 * @return int 
 */
int rdtsc_clocksource_calibrate(void);

#endif // TSCCLOCKSOURCE_INCLUDE_H