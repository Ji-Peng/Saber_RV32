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

void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYCOINBYTES];
    int32_t i;

    shake128(buf, sizeof(buf), seed, SABER_NOISE_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        cbd(s[i], buf + i * SABER_POLYCOINBYTES, SABER_N);
    }
}

void GenSecretInTime(uint16_t s[SABER_N],
                     const uint8_t seed[SABER_NOISE_SEEDBYTES], int32_t index)
{
    int32_t i;
    // keccak states
    static uint64_t keccak_state[25];

    if (index == 0) {
        // init
        for (i = 0; i < 25; i++)
            keccak_state[i] = 0;
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, SABER_NOISE_SEEDBYTES,
                      0x1F);
    }

#if SABER_MU == 6
    uint8_t buf[SHAKE128_RATE];
    static uint8_t leftovers[72];
    if (index == 0) {
        // 1buf = 224coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s, buf, 224);
        // 1buf = 32coeff + 72B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 224, buf, 32);
        memcpy(leftovers, buf + SHAKE128_RATE - sizeof(leftovers),
               sizeof(leftovers));
    } else if (index == 1) {
        // 1leftover = 72B = 96coeff
        cbd(s, leftovers, 96);
        // 1buf = 160coeff + 48B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 96, buf, 160);
        memcpy(leftovers, buf + SHAKE128_RATE - 48, 48);
    } else if (index == 2) {
        // 48B leftover = 64coeff
        cbd(s, leftovers, 64);
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 64, buf, 192);
        memcpy(leftovers, buf + SHAKE128_RATE - 24, 24);
    } else {
        // 24B leftover = 32coeff
        cbd(s, leftovers, 32);
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 32, buf, 224);
    }

#elif SABER_MU == 8
    uint8_t buf[SHAKE128_RATE];
    static uint8_t leftovers[88];
    if (index == 0) {
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s, buf, 168);
        // 1buf = 88coeff + 80B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 168, buf, 88);
        memcpy(leftovers, buf + SHAKE128_RATE - 80, 80);
    } else if (index == 1) {
        // 1leftover = 80B = 80coeff
        cbd(s, leftovers, 80);
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 80, buf, 168);
        // 1buf = 8coeff + 88B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 248, buf, 8);
        memcpy(leftovers, buf + SHAKE128_RATE - 88, 88);
    } else {
        // 1leftover = 88B = 88coeff
        cbd(s, leftovers, 88);
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        cbd(s + 88, buf, 168);
    }

#elif SABER_MU == 10
    uint8_t buf[SHAKE128_RATE];
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    cbd(s, buf, SABER_N / 2);
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    cbd(s + SABER_N / 2, buf, SABER_N / 2);
#else
#    error "Unsupported SABER parameter."
#endif
}

void GenSecret_ntt(int32_t s[SABER_L][SABER_N],
                   const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint16_t t[SABER_N];
    int i;

    for (i = 0; i < SABER_L; i++) {
        GenSecretInTime(t, seed, i);
        ntt(t, s[i]);
    }
}

/*************************************************
 * Name:        poly_basemul
 *
 * Description: Multiplication of two polynomials in NTT domain
 *
 * Arguments:   - a: pointer to first input polynomial and also output
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
 * Description: res += a * b using ntt，a, b in standard domain
 */
void poly_mul_acc_ntt(uint16_t a[2 * SABER_N], const uint16_t b[SABER_N],
                      uint16_t res[SABER_N])
{
    int32_t t[SABER_N];
    int32_t *p = (int32_t *)a;
    ntt(a, t);
    ntt(b, p);
    poly_basemul(p, t);
    invntt(p, t);
    poly_add(res, t);
}

/**
 * Name: poly_mul_acc_ntt
 * Description: res += a * b using ntt，a in standard domain, b in ntt domain
 */
void poly_mul_acc_ntt_fast(uint16_t a[SABER_N], const int32_t b[SABER_N],
                           uint16_t res[SABER_N])
{
    int32_t t[SABER_N];
    int32_t *pb = (int32_t *)b;
    ntt(a, t);
    poly_basemul(t, pb);
    invntt(t, t);
    poly_add(res, t);
}

#ifdef SLOWGENA_FASTMUL
/**
 * @description: Generate polynomial matrix A on the fly
 */
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             int32_t x, int32_t y)
{
    int32_t i;
    uint8_t buf[SHAKE128_RATE];
    // can generate 50 coefficients
    static uint8_t leftovers[82];
    // state = 0 or 1, 0: store leftovers; 1: load leftovers
    static uint32_t state;
    // keccak states
    static uint64_t keccak_state[25];
    // init: clear states and absorb seed
    if (init == 1) {
        for (i = 0; i < 25; i++)
            keccak_state[i] = 0;
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);
        state = 0;
    }
    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 96);

    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly + 103, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 199);

    // 0: save to leftovers or 1: load from leftovers
    if (state == 0) {
        // squeeze output and generate 50 coefficients
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        // 48coeff = 78bytes
        BS2POLq(buf, poly + 206, 48);
        // 2coeff = 26bits(4bytes)
        BS2POLq2(buf + 78, poly + 254);
        memcpy(leftovers, &buf[82], 82);
    } else {
        // use leftovers to generate 50 coefficients
        BS2POLq(leftovers, poly + 206, 48);
        BS2POLq2(leftovers + 78, poly + 254);
    }
    state = !state;
}
#else
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
    // state = 0 or 1, 0: store leftovers; 1: load leftovers
    static uint32_t state;
    // keccak states
    static uint64_t keccak_state[25];
    // init: clear states and absorb seed
    if (init == 1) {
        for (i = 0; i < 25; i++)
            keccak_state[i] = 0;
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);
        state = 0;
    }
    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 96);

    // squeeze output and generate 103 coefficients
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    // 96coeff = 156bytes
    BS2POLq(buf, poly + 103, 96);
    // 7coeff = 91bits(12bytes)
    BS2POLq7(buf + 156, poly + 199);

    // 0: save to leftovers or 1: load from leftovers
    if (state == 0) {
        // squeeze output and generate 50 coefficients
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        // 48coeff = 78bytes
        BS2POLq(buf, poly + 206, 48);
        // 2coeff = 26bits(4bytes)
        BS2POLq2(buf + 78, poly + 254);
        memcpy(leftovers, &buf[82], 82);
    } else {
        // use leftovers to generate 50 coefficients
        BS2POLq(leftovers, poly + 206, 48);
        BS2POLq2(leftovers + 78, poly + 254);
    }
    state = !state;
}
#endif

void MatrixVectorMulKP_ntt(const uint8_t *seed_a, const uint8_t *seed_s,
                           uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                           uint16_t b[SABER_L][SABER_N])
{
    int i, j;
    uint16_t t2[SABER_N];
    int32_t t1[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // t2=si
        GenSecretInTime(t2, seed_s, i);
        // pack si to sk
        pack_sk(sk + i * SABER_SKPOLYBYTES, t2);
        // trans si to ntt domain, which is saved in t1
        ntt(t2, t1);
        // generate poly and muladd
        for (j = 0; j < SABER_L; j++) {
#ifdef SLOWGENA_FASTMUL
            GenPoly(t2, seed_a, i, j);
#else
            // i=0, j=0, init=1, t2=aij
            GenPoly(t2, seed_a, 1 - i - j);
#endif
            poly_mul_acc_ntt_fast(t2, t1, b[j]);
        }
    }
}

#ifdef FASTGENA_SLOWMUL
void MatrixVectorMulEnc_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext)
{
    int i, j;
    uint16_t a[2 * SABER_N], res[SABER_N];
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

#elif defined(FASTGENA_FASTMUL)
void MatrixVectorMulEnc_ntt(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
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
            poly_mul_acc_ntt_fast(a, s[j], res);
        }
        for (j = 0; j < SABER_N; j++) {
            res[j] = (res[j] + h1) >> (SABER_EQ - SABER_EP);
        }
        POLp2BS(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
}

#else

#endif
/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt
 */
void InnerProdInTime_ntt(const uint8_t *bytes,
                         const uint16_t s[SABER_L][SABER_N],
                         uint16_t res[SABER_N])
{
    int j;
    uint16_t b[2 * SABER_N];

    for (j = 0; j < SABER_L; j++) {
        BS2POLp(bytes + j * (SABER_EP * SABER_N / 8), b);
        poly_mul_acc_ntt(b, s[j], res);
    }
}

#ifdef FASTGENA_SLOWMUL
/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt
 */
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                            const uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext,
                            const uint8_t m[SABER_KEYBYTES])
{
    int i, j;
    uint16_t b[2 * SABER_N], vp[SABER_N] = {0};
    uint16_t message_bit;

    for (j = 0; j < SABER_L; j++) {
        BS2POLp(bytes + j * (SABER_EP * SABER_N / 8), b);
        poly_mul_acc_ntt(b, s[j], vp);
    }
    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            message_bit = ((m[j] >> i) & 0x01);
            message_bit = (message_bit << (SABER_EP - 1));
            vp[j * 8 + i] =
                (vp[j * 8 + i] - message_bit + h1) >> (SABER_EP - SABER_ET);
        }
    }

    POLT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
}
#elif defined(FASTGENA_FASTMUL)
/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt, s in ntt domain
 */
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                                 const int32_t s[SABER_L][SABER_N],
                                 uint8_t *ciphertext,
                                 const uint8_t m[SABER_KEYBYTES])
{
    int i, j;
    uint16_t b[SABER_N], vp[SABER_N] = {0};
    uint16_t message_bit;

    for (j = 0; j < SABER_L; j++) {
        BS2POLp(bytes + j * (SABER_EP * SABER_N / 8), b);
        poly_mul_acc_ntt_fast(b, s[j], vp);
    }
    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            message_bit = ((m[j] >> i) & 0x01);
            message_bit = (message_bit << (SABER_EP - 1));
            vp[j * 8 + i] =
                (vp[j * 8 + i] - message_bit + h1) >> (SABER_EP - SABER_ET);
        }
    }

    POLT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
}
#else
#endif