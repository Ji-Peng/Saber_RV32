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

/**
 * @description: Generate polynomial in time
 * @param: nblocks = 2 or 3
 *  if nblocks==3: squeeze 3 blocks -> 168*3=504B -> use 418B to generate poly
 * -> mov remaining 88B to static leftovers
 *
 * if nblocks==2: squeeze 2 blocks -> 168*2=336B -> merge with leftovers 88B and
 * get 424B
 */
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             uint8_t init, uint8_t nblocks)
{
    uint8_t i;
    uint8_t buf[SHAKE128_RATE * 3];
    static uint8_t leftovers[SHAKE128_RATE * 3 - SABER_POLYBYTES];
    // keccak states
    static uint64_t s[25];

    // init: clear states and absorb seed
    if (init == 1) {
        for (i = 0; i < 25; i++)
            s[i] = 0;
        keccak_absorb(s, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);
    }
    // squeeze output and generate polynomial
    keccak_squeezeblocks(buf, nblocks, s, SHAKE128_RATE);
    if (nblocks == 3) {
        // move remaining 88bytes to static leftovers
        memcpy(leftovers, buf + SABER_POLYBYTES, sizeof(leftovers));
    } else if (nblocks == 2) {
        //  merge with leftovers
        memcpy(buf + SHAKE128_RATE * 2, leftovers, sizeof(leftovers));
    } else {
        printf("ERROR in GenPoly\n");
        exit(1);
    }
    BS2POLq(buf, poly);
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

/**
 * @description: MatrixVectorMul just-in-time
 * nblocks = 3 2 3 2 3 2 3 2 3 corresponding to ij = 00 01 02 10 11 12 20
 * 21 22, so when ij = 00 02 11 20 22, (i+j)&1=0, nblocks=3-(i+j)&1=3, when
 * ij=01, 10, 12, 21, (i+j)&1=1, nblocks=3-(i+j)&1=2
 */
void MatrixVectorMulKP_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                           uint16_t b[SABER_L][SABER_N])
{
    int i, j;
    uint16_t a[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // generate poly and muladd
        for (j = 0; j < SABER_L; j++) {
            GenPoly(a, seed, 1 - i - j, 3 - ((i + j) & 0x01));
            // printf("-GenPoly\n");
            poly_mul_acc_ntt(a, s[i], b[j]);
            // printf("-poly_mul_acc_ntt\n");
        }
    }
}

/**
 * @description: MatrixVectorMul just-in-time
 * nblocks = 3 2 3 2 3 2 3 2 3 corresponding to ij = 00 01 02 10 11 12 20
 * 21 22, so when ij = 00 02 11 20 22, (i+j)&1=0, nblocks=3-(i+j)&1=3, when
 * ij=01, 10, 12, 21, (i+j)&1=1, nblocks=3-(i+j)&1=2
 */
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
            GenPoly(a, seed, 1 - i - j, 3 - ((i + j) & 0x01));
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