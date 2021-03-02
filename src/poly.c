#include "poly.h"

#include <stdio.h>

#include "api.h"
#include "cbd.h"
#include "fips202.h"
#include "ntt.h"
#include "pack_unpack.h"
#include "poly_mul.h"

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
    for (i = 0; i < SABER_N; i++) {
        r[i] = fqmul(a[i], b[i]);
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