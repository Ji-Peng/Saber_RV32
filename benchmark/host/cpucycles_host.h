#ifndef CPUCYCLES_H
#define CPUCYCLES_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void print_results(const char *s, uint64_t *t, size_t tlen);
uint64_t cpucycles_overhead(void);

static inline uint64_t cpucycles(void)
{
    uint64_t result;

    __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     :
                     : "%rdx");

    return result;
}

#endif