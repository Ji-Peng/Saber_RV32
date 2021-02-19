#include "poly.h"

#include <stdio.h>

#include "api.h"
#include "cbd.h"
#include "fips202.h"
#include "ntt.h"
#include "pack_unpack.h"
#include "poly_mul.h"

// point-wise multiplication mod (X^4-zeta^{2br(i)+1}) i=0,1,...,63
const int32_t mul_table[64] = {
    -9600669, 9600669,   10575964,  -10575964, 8064557,   -8064557,  -819256,
    819256,   -588496,   588496,    -8693794,  8693794,   -7460755,  7460755,
    2723061,  -2723061,  4092287,   -4092287,  -3261033,  3261033,   -5563113,
    5563113,  -11307548, 11307548,  -9567042,  9567042,   11980428,  -11980428,
    6931502,  -6931502,  2510833,   -2510833,  -10319196, 10319196,  -6726360,
    6726360,  10171507,  -10171507, 8693725,   -8693725,  -42688,    42688,
    10505644, -10505644, -9502337,  9502337,   10910265,  -10910265, 5318976,
    -5318976, -1134236,  1134236,   -614272,   614272,    -6236460,  6236460,
    5184115,  -5184115,  -1069349,  1069349,   -9233574,  9233574,   12174351,
    -12174351};

void MatrixVectorMul(const uint16_t A[SABER_L][SABER_L][SABER_N],
                     const uint16_t s[SABER_L][SABER_N],
                     uint16_t res[SABER_L][SABER_N], int16_t transpose)
{
    int i, j;
    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_L; j++) {
            if (transpose == 1) {
                poly_mul_acc(A[j][i], s[j], res[i]);
            } else {
                poly_mul_acc(A[i][j], s[j], res[i]);
            }
        }
    }
}

void InnerProd(const uint16_t b[SABER_L][SABER_N],
               const uint16_t s[SABER_L][SABER_N], uint16_t res[SABER_N])
{
    int j;
    for (j = 0; j < SABER_L; j++) {
        poly_mul_acc(b[j], s[j], res);
    }
}

void GenMatrix(uint16_t A[SABER_L][SABER_L][SABER_N],
               const uint8_t seed[SABER_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYVECBYTES];
    int i;

    shake128(buf, sizeof(buf), seed, SABER_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        BS2POLVECq(buf + i * SABER_POLYVECBYTES, A[i]);
    }
}

void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYCOINBYTES];
    size_t i;

    shake128(buf, sizeof(buf), seed, SABER_NOISE_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        cbd(s[i], buf + i * SABER_POLYCOINBYTES);
    }
}

/*************************************************
 * Name:        poly_basemul
 *
 * Description: Multiplication of two polynomials in NTT domain
 *
 * Arguments:   - r: pointer to output polynomial
 *              - a: pointer to first input polynomial
 *              - b: pointer to second input polynomial
 **************************************************/
void poly_basemul(int32_t r[SABER_N], const int32_t a[SABER_N],
                  const int32_t b[SABER_N])
{
    unsigned int i;
    for (i = 0; i < SABER_N / 4; i++) {
        basemul(&r[4 * i], &a[4 * i], &b[4 * i], mul_table[i]);
    }
}

/**
 * Name: poly_add
 * Description: polynomial addition
 */
void poly_add(int16_t res[SABER_N], int32_t in[SABER_N])
{
    int i;
    for (i = 0; i < SABER_N; i++) {
        res[i] += (int16_t)in[i];
    }
}

/**
 * Name: poly_mul_acc_ntt
 * Description: res += a * b using ntt
 */
void poly_mul_acc_ntt(const int16_t a[SABER_N], const int16_t b[SABER_N],
                      int16_t res[SABER_N])
{
    int32_t t1[SABER_N], t2[SABER_N], t3[SABER_N];
    ntt(a, t1);
    ntt(b, t2);
    poly_basemul(t3, t1, t2);
    invntt(t3, t1);
    poly_add(res, t1);
}

/**
 * Name: InnerProd_ntt
 * Description: inner product using ntt
 */
void InnerProd_ntt(const int16_t b[SABER_L][SABER_N],
                   const int16_t s[SABER_L][SABER_N], int16_t res[SABER_N])
{
    int j;
    for (j = 0; j < SABER_L; j++) {
        poly_mul_acc_ntt(b[j], s[j], res);
    }
}

void MatrixVectorMul_ntt(const int16_t A[SABER_L][SABER_L][SABER_N],
                         const int16_t s[SABER_L][SABER_N],
                         int16_t res[SABER_L][SABER_N], int16_t transpose)
{
    int i, j;
    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_L; j++) {
            if (transpose == 1) {
                poly_mul_acc_ntt(A[j][i], s[j], res[i]);
            } else {
                poly_mul_acc_ntt(A[i][j], s[j], res[i]);
            }
        }
    }
}