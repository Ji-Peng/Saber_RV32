#include "poly.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "cbd.h"
#include "fips202.h"
#include "ntt.h"
#include "pack_unpack.h"
#include "poly_mul.h"

// point-wise multiplication mod (X^4-zeta^{2br(i)+1}) i=0,1,...,63
const int32_t mul_table[64] = {
    -136014,  136014,   966523,   -966523,  959523,   -959523,  846643,
    -846643,  -86562,   86562,    -489847,  489847,   136654,   -136654,
    -2088895, 2088895,  17941,    -17941,   -1051723, 1051723,  -1316589,
    1316589,  1814059,  -1814059, -230501,  230501,   1626667,  -1626667,
    -1171284, 1171284,  2085817,  -2085817, 1830521,  -1830521, -1951522,
    1951522,  445122,   -445122,  -1689285, 1689285,  -1551600, 1551600,
    -2055310, 2055310,  -1064338, 1064338,  -368446,  368446,   535845,
    -535845,  361370,   -361370,  676319,   -676319,  -541241,  541241,
    1009639,  -1009639, 538875,   -538875,  -2102677, 2102677,  1585701,
    -1585701};

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

// void GenMatrix(uint16_t A[SABER_L][SABER_L][SABER_N],
//                const uint8_t seed[SABER_SEEDBYTES])
// {
//     uint8_t buf[SABER_L * SABER_POLYVECBYTES];
//     int i;

//     shake128(buf, sizeof(buf), seed, SABER_SEEDBYTES);

//     for (i = 0; i < SABER_L; i++) {
//         BS2POLVECq(buf + i * SABER_POLYVECBYTES, A[i]);
//     }
// }

/**
 * @description: Generate polynomial on the fly
 */
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             uint32_t init)
{
    int32_t i;
    uint8_t buf[SHAKE128_RATE];
    // can generate 50 coefficients
    static uint8_t leftovers[82];
    // keccak states
    static uint64_t s[25];
    // state = 0 or 1, 0: store leftovers; 1: load leftovers
    static uint32_t state;

    // init: clear states and absorb seed
    if (init == 1) {
        for (i = 0; i < 25; i++)
            s[i] = 0;
        keccak_absorb(s, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);
        state = 0;
    }
    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, s, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 96);

    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, s, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly + 103, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 199);

    // 0: save to leftovers or 1: load from leftovers
    if (state == 0) {
        // squeeze output and generate 50 coefficients
        keccak_squeezeblocks(buf, 1, s, SHAKE128_RATE);
        // 48coeff = 78bytes
        BS2POLq(buf, poly + 206, 48);
        // 2coeff = 26bits(4bytes)
        BS2POLq2(buf + 78, poly + 254);
        memcpy(leftovers, buf + 82, sizeof(leftovers));
    } else {
        // use leftovers to generate 50 coefficients
        BS2POLq(leftovers, poly + 206, 48);
        BS2POLq2(leftovers + 78, poly + 254);
    }
    state = !state;
}

void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYCOINBYTES];
    int32_t i;

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
void poly_basemul(int32_t a[SABER_N], const int32_t b[SABER_N])
{
    unsigned int i;
    for (i = 0; i < SABER_N / 4; i++) {
        basemul(&a[4 * i], &b[4 * i], mul_table[i]);
    }
}

/**
 * Name: poly_add
 * Description: polynomial addition
 */
void poly_add(uint16_t res[SABER_N], int32_t in[SABER_N])
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
void poly_mul_acc_ntt(const uint16_t a[SABER_N], const uint16_t b[SABER_N],
                      uint16_t res[SABER_N])
{
    int32_t t1[SABER_N], t2[SABER_N];
    ntt(a, t1);
    // printf("--ntt\n");
    ntt(b, t2);
    // printf("--ntt\n");
    poly_basemul(t1, t2);
    // printf("--poly_basemul\n");
    invntt(t1, t2);
    // printf("--invntt\n");
    poly_add(res, t2);
    // printf("--poly_add\n");
}

/**
 * Name: InnerProd_ntt
 * Description: inner product using ntt
 */
// void InnerProd_ntt(const int16_t b[SABER_L][SABER_N],
//                    const int16_t s[SABER_L][SABER_N], int16_t res[SABER_N])
// {
//     int j;
//     for (j = 0; j < SABER_L; j++) {
//         poly_mul_acc_ntt(b[j], s[j], res);
//     }
// }

// void MatrixVectorMul_ntt(const int16_t A[SABER_L][SABER_L][SABER_N],
//                          const int16_t s[SABER_L][SABER_N],
//                          int16_t res[SABER_L][SABER_N], int16_t transpose)
// {
//     int i, j;
//     for (i = 0; i < SABER_L; i++) {
//         for (j = 0; j < SABER_L; j++) {
//             if (transpose == 1) {
//                 poly_mul_acc_ntt(A[j][i], s[j], res[i]);
//             } else {
//                 poly_mul_acc_ntt(A[i][j], s[j], res[i]);
//             }
//         }
//     }
// }

void MatrixVectorMulKP_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                           uint16_t b[SABER_L][SABER_N])
{
    int i, j;
    uint16_t a[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // generate poly and muladd
        for (j = 0; j < SABER_L; j++) {
            // i=0, j=0, init=1
            GenPoly(a, seed, 1 - i - j);
            // printf("-GenPoly\n");
            poly_mul_acc_ntt(a, s[i], b[j]);
            // printf("-poly_mul_acc_ntt\n");
        }
    }
}

void MatrixVectorMulEnc_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext)
{
    int i, j;
    uint16_t a[SABER_N], res[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // clear a and res
        for (j = 0; j < SABER_N; j++) {
            a[j] = 0;
            res[j] = 0;
        }
        // generate poly and muladd: res=A[i0]*s[0]+A[i1]*s[1]+A[i2]*s[2]
        for (j = 0; j < SABER_L; j++) {
            GenPoly(a, seed, 1 - i - j);
            poly_mul_acc_ntt(a, s[j], res);
        }
        for (j = 0; j < SABER_N; j++) {
            res[j] = (res[j] + h1) >> (SABER_EQ - SABER_EP);
        }
        POLp2BS(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
}

/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt
 */
void InnerProdInTime_ntt(const uint8_t *bytes,
                         const uint16_t s[SABER_L][SABER_N],
                         uint16_t res[SABER_N])
{
    int j;
    uint16_t b[SABER_N];

    for (j = 0; j < SABER_L; j++) {
        BS2POLp(bytes + j * (SABER_EP * SABER_N / 8), b);
        poly_mul_acc_ntt(b, s[j], res);
    }
}