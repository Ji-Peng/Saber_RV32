#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles.h"
#include "fips202.h"
#include "ntt.h"
#include "poly.h"
#include "rng.h"

void test_ntt(void)
{
    int16_t in[256];
    int32_t out1[256], out2[256];
    uint64_t t1, t2;
    t1 = cpucycles();
    ntt(in, out1);
    t2 = cpucycles();
    printf("ntt cycles is %s\n", ullu(t2 - t1));
    t1 = cpucycles();
    ntt_merged(in, out2);
    t2 = cpucycles();
    printf("ntt_merged cycles is %s\n", ullu(t2 - t1));
}

int main(void)
{
    test_ntt();
}