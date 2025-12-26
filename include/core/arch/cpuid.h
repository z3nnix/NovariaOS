#ifndef _CPUID_H
#define _CPUID_H

#include <stdint.h>

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result_t;

void cpuid(uint32_t leaf, uint32_t subleaf, cpuid_result_t* result);

#endif