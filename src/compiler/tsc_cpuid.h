#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
#include <cpuid.h>

enum cpu_register_t {
	REG_EAX = 0,
	REG_EBX,
	REG_ECX,
	REG_EDX,
};

typedef uint32_t cpuid_registers_t[4];

#endif /* CPUID_H */