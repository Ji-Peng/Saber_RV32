#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles.h"
#include "fips202.h"
#include "ntt.h"
#include "pack_unpack.h"
#include "poly.h"
#include "poly_mul.h"
#include "rng.h"

#define NTESTS 1

static void test_ntt(void)
{
    int16_t in[256];
    int32_t out1[256], out2[256];
    uint64_t t1, t2;
    t1 = cpucycles();
    ntt(in, out1);
    t2 = cpucycles();
    printf("ntt cycles is %s\n", ullu(t2 - t1));
}

static void test_poly_mul(void)
{
    int16_t a[SABER_N];
    int16_t b[SABER_N];
    int16_t res[SABER_N];
    uint64_t t1, t2, sum;
    printf("---------TESTING-----------\n");
    sum = 0;
    for (int i = 0; i < NTESTS; i++) {
        t1 = cpucycles();
        poly_mul_acc(a, b, res);
        t2 = cpucycles();
        sum += (t2 - t1);
    }
    printf("poly_mul_acc cycles is %s\n", ullu(sum / NTESTS));
    sum = 0;
    for (int i = 0; i < NTESTS; i++) {
        t1 = cpucycles();
        poly_mul_acc_ntt(a, b, res);
        t2 = cpucycles();
        sum += (t2 - t1);
    }
    printf("poly_mul_acc_ntt cycles is %s\n", ullu(sum / NTESTS));
    printf("NTESTS is %d\n", NTESTS);
    printf("---------TEST END----------\n");
}


int main(void)
{
    // test_ntt();
    // test_poly_mul();
    // test_pack();
}