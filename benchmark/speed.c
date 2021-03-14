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

// static void test_pack(void)
// {
//     uint16_t s1[SABER_L][SABER_N], s2[SABER_L][SABER_N];
//     uint8_t sk1[SABER_INDCPA_SECRETKEYBYTES],
//         sk2[SABER_INDCPA_SECRETKEYBYTES + 9 * 3 * 256 / 8];
//     int32_t i, j;
//     uint64_t t1, t2, sum;

//     for (i = 0; i < SABER_L; i++)
//         for (j = 0; j < SABER_N; j++) {
//             s1[i][j] = s2[i][j] = j;
//         }
//     printf("---------TESTING-----------\n");
//     sum = 0;
//     for (int i = 0; i < NTESTS; i++) {
//         t1 = cpucycles();
//         pack_sk(sk1, s1);
//         t2 = cpucycles();
//         sum += (t2 - t1);
//     }
//     printf("pack_sk cycles is %s\n", ullu(sum / NTESTS));

//     sum = 0;
//     for (int i = 0; i < NTESTS; i++) {
//         t1 = cpucycles();
//         unpack_sk(sk1, s1);
//         t2 = cpucycles();
//         sum += (t2 - t1);
//     }
//     printf("unpack_sk cycles is %s\n", ullu(sum / NTESTS));

//     sum = 0;
//     for (int i = 0; i < NTESTS; i++) {
//         t1 = cpucycles();
//         POLVECq2BS(sk2, s2);
//         t2 = cpucycles();
//         sum += (t2 - t1);
//     }
//     printf("POLVECq2BS cycles is %s\n", ullu(sum / NTESTS));

//     sum = 0;
//     for (int i = 0; i < NTESTS; i++) {
//         t1 = cpucycles();
//         BS2POLVECq(sk2, s2);
//         t2 = cpucycles();
//         sum += (t2 - t1);
//     }
//     printf("BS2POLVECq cycles is %s\n", ullu(sum / NTESTS));
//     printf("---------TEST END----------\n");
// }

int main(void)
{
    // test_ntt();
    // test_poly_mul();
    // test_pack();
}