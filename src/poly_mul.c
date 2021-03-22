#include "poly_mul.h"

#include <stdint.h>
#include <string.h>

#include "ntt.h"
#include "pack_unpack.h"
#include "poly.h"

// point-wise multiplication mod (X^4-zeta^{2br(i)+1}) i=0,1,...,63
const int32_t mulTable[64] = {
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

/*************************************************
 * Name:        PolyBaseMul
 *
 * Description: Multiplication of two polynomials in NTT domain
 *
 * Arguments:   - a: pointer to first input polynomial and also output
 *              - b: pointer to second input polynomial
 **************************************************/
void PolyBaseMul(int32_t a[SABER_N], const int32_t b[SABER_N])
{
    unsigned int i;
    for (i = 0; i < SABER_N / 4; i++) {
        BaseMul(&a[4 * i], &b[4 * i], mulTable[i]);
    }
}

/**
 * Name: PolyAdd
 * Description: polynomial addition
 */
void PolyAdd(uint16_t res[SABER_N], int32_t in[SABER_N])
{
    int i;
    for (i = 0; i < SABER_N; i++) {
        res[i] += (int16_t)in[i];
    }
}

/**
 * Name: PolyMulAcc
 * Description: res += a * b using ntt，a, b in standard domain
 */
__attribute__((noinline)) void PolyMulAcc(uint16_t a[2 * SABER_N],
                                          const uint16_t b[SABER_N],
                                          uint16_t res[SABER_N])
{
    int32_t t[SABER_N];
    int32_t *p = (int32_t *)a;
    NTT(a, t);
    NTT(b, p);
    PolyBaseMul(p, t);
    InvNTT(p, t);
    PolyAdd(res, t);
}

/**
 * Name: PolyMulAcc
 * Description: res += a * b using ntt，a in standard domain, b in ntt domain
 */
__attribute__((noinline)) void PolyMulAccFast(uint16_t a[SABER_N],
                                              const int32_t b[SABER_N],
                                              uint16_t res[SABER_N])
{
    int32_t t[SABER_N];
    int32_t *pb = (int32_t *)b;
    NTT(a, t);
    PolyBaseMul(t, pb);
    InvNTT(t, t);
    PolyAdd(res, t);
}

void MatrixVectorMulKP(const uint8_t *seed_a, const uint8_t *seed_s,
                       uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                       uint16_t b[SABER_L][SABER_N])
{
    int i, j;
    int32_t t1[SABER_N];
    uint16_t t2[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // t2=si
        GenSInTime(t2, seed_s, i);
        // pack si to sk
        PackSk(sk + i * SABER_SKPOLYBYTES, t2);
        // trans si to ntt domain, which is saved in t1
        NTT(t2, t1);
        // generate poly and muladd
        for (j = 0; j < SABER_L; j++) {
#if defined(FASTGENA_SLOWMUL) || defined(FASTGENA_FASTMUL)
            // i=0, j=0, init=1, t2=aij
            GenAInTime(t2, seed_a, 1 - i - j);
#elif defined(SLOWGENA_FASTMUL)
            GenAInTime(t2, seed_a, i, j);
#endif
            PolyMulAccFast(t2, t1, b[j]);
        }
    }
}

#ifdef FASTGENA_SLOWMUL
void MatrixVectorMulEnc(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
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
            GenAInTime(a, seed, 1 - i - j);
            PolyMulAcc(a, s[j], res);
        }
        for (j = 0; j < SABER_N; j++) {
            res[j] = (res[j] + h1) >> (SABER_EQ - SABER_EP);
        }
        Polp2BS(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
}

/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt
 */
void InnerProdInTimeEnc(const uint8_t *bytes,
                        const uint16_t s[SABER_L][SABER_N], uint8_t *ciphertext,
                        const uint8_t m[SABER_KEYBYTES])
{
    int i, j;
    uint16_t b[2 * SABER_N], vp[SABER_N] = {0};
    uint16_t message_bit;

    for (j = 0; j < SABER_L; j++) {
        BS2Polp(bytes + j * (SABER_EP * SABER_N / 8), b);
        PolyMulAcc(b, s[j], vp);
    }
    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            message_bit = ((m[j] >> i) & 0x01);
            message_bit = (message_bit << (SABER_EP - 1));
            vp[j * 8 + i] =
                (vp[j * 8 + i] - message_bit + h1) >> (SABER_EP - SABER_ET);
        }
    }

    PolT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
}

#elif defined(FASTGENA_FASTMUL)
void MatrixVectorMulEnc(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
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
            GenAInTime(a, seed, 1 - i - j);
            PolyMulAccFast(a, s[j], res);
        }
        for (j = 0; j < SABER_N; j++) {
            res[j] = (res[j] + h1) >> (SABER_EQ - SABER_EP);
        }
        Polp2BS(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
}

/**
 * Name: InnerProd just-in-time
 * Description: inner product using ntt, s in ntt domain
 */
void InnerProdInTimeEnc(const uint8_t *bytes, const int32_t s[SABER_L][SABER_N],
                        uint8_t *ciphertext, const uint8_t m[SABER_KEYBYTES])
{
    int i, j;
    uint16_t b[SABER_N], vp[SABER_N] = {0};
    uint16_t message_bit;

    for (j = 0; j < SABER_L; j++) {
        BS2Polp(bytes + j * (SABER_EP * SABER_N / 8), b);
        PolyMulAccFast(b, s[j], vp);
    }
    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            message_bit = ((m[j] >> i) & 0x01);
            message_bit = (message_bit << (SABER_EP - 1));
            vp[j * 8 + i] =
                (vp[j * 8 + i] - message_bit + h1) >> (SABER_EP - SABER_ET);
        }
    }

    PolT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
}

#else

#endif

void InnerProdInTimeDec(const uint8_t *bytes,
                        const uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                        uint16_t res[SABER_N])
{
    int j;
    uint16_t b[2 * SABER_N];
    uint16_t s[SABER_N];

    for (j = 0; j < SABER_L; j++) {
        BS2Polp(bytes + j * (SABER_EP * SABER_N / 8), b);
        UnpackSk(sk + j * SABER_SKPOLYBYTES, s);
        PolyMulAcc(b, s, res);
    }
}