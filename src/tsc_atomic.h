#ifndef TSC_ATOMIC_H
#define TSC_ATOMIC_H

#define tsc_lfence() asm volatile("lfence":::"memory")

#endif // TSC_ATOMIC_H