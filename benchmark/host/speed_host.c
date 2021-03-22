#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles_host.h"
#include "fips202.h"
#include "ntt.h"
#include "poly.h"
#include "poly_mul.h"
#include "rng.h"

static int TestNTT(void);
static int test_ntt2(void);
void test_poly_mul(void);

#define NTESTS 10000

uint64_t t[NTESTS];

static int TestNTT(void)
{
    uint16_t A[SABER_L][SABER_L][SABER_N];
    uint16_t s[SABER_L][SABER_N];
    uint16_t b[SABER_L][SABER_N] = {0};
    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    int i;
    unsigned char entropy_input[48];

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    RandomBytesInit(entropy_input, NULL);

    RandomBytes(seed_A, SABER_SEEDBYTES);
    shake128(seed_A, SABER_SEEDBYTES, seed_A,
             SABER_SEEDBYTES);  // for not revealing system RNG state
    RandomBytes(seed_s, SABER_NOISE_SEEDBYTES);

    GenSecret(s, seed_s);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        MatrixVectorMul_ntt((int16_t(*)[3][256])A, (int16_t(*)[256])s,
                            (int16_t(*)[256])b, 1);
    }
    print_results("MatrixVectorMul_ntt: ", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        MatrixVectorMul(A, s, b, 1);
    }
    print_results("MatrixVectorMul: ", t, NTESTS);
    return 0;
}

static int test_ntt2(void)
{
    int16_t in[256];
    int32_t out1[256];
    int32_t i;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        NTT(in, out1);
    }
    print_results("NTT: ", t, NTESTS);

    return 0;
}

void test_poly_mul(void)
{
    int16_t a[SABER_N];
    int16_t b[SABER_N];
    int16_t res[SABER_N];
    uint64_t t1, t2;
    t1 = cpucycles();
    PolyMulAcc((uint16_t*)a, (uint16_t*)b, (uint16_t*)res);
    t2 = cpucycles();
    printf("PolyMulAcc cycles is %lu\n", (t2 - t1));
    t1 = cpucycles();
    PolyMulAcc(a, b, res);
    t2 = cpucycles();
    printf("PolyMulAcc cycles is %lu\n", (t2 - t1));
}

int main(void)
{
    TestNTT();
    test_ntt2();
    test_poly_mul();
}
