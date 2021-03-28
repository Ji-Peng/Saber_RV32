#include "poly_mul.h"

#include <stdint.h>
#include <string.h>

#include "ntt.h"
#include "pack_unpack.h"
#include "poly.h"

// point-wise multiplication mod (X^4-zeta^{2br(i)+1}) i=0,1,...,63
const int32_t mulTable[64] = {
    2896842,  -2896842, 1150913,  -1150913, 1250,     -1250,    -771147,
    771147,   -3214001, 3214001,  -2216070, 2216070,  2587567,  -2587567,
    4635835,  -4635835, 1247022,  -1247022, 1133020,  -1133020, 1610224,
    -1610224, -4652015, 4652015,  4035904,  -4035904, 254135,   -254135,
    2640679,  -2640679, 1621924,  -1621924, -2966437, 2966437,  -1300452,
    1300452,  -3963361, 3963361,  3815660,  -3815660, -4635716, 4635716,
    -4810532, 4810532,  394299,   -394299,  -3565801, 3565801,  723646,
    -723646,  1340759,  -1340759, 1171195,  -1171195, 4777770,  -4777770,
    -495362,  495362,   -1032438, 1032438,  -4833797, 4833797,  152199,
    -152199};

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

int32_t MatrixVectorMulEncCmp(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                              const uint8_t *ciphertext)
{
    int i, j, fail = 0;
    uint16_t a[2 * SABER_N], res[SABER_N];
    for (i = 0; i < SABER_L; i++) {
        // clear a and res
        for (j = 0; j < SABER_N; j++) {
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
        fail |= Polp2BSCmp(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
    return fail;
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

int32_t InnerProdInTimeEncCmp(const uint8_t *bytes,
                              const uint16_t s[SABER_L][SABER_N],
                              const uint8_t *ciphertext,
                              const uint8_t m[SABER_KEYBYTES])
{
    int i, j, fail = 0;
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

    fail |= PolT2BSCmp(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
    return fail;
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

int32_t MatrixVectorMulEncCmp(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
                              const uint8_t *ciphertext)
{
    int i, j, fail = 0;
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
        fail |= Polp2BSCmp(ciphertext + i * (SABER_EP * SABER_N / 8), res);
    }
    return fail;
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

int32_t InnerProdInTimeEncCmp(const uint8_t *bytes,
                              const int32_t s[SABER_L][SABER_N],
                              const uint8_t *ciphertext,
                              const uint8_t m[SABER_KEYBYTES])
{
    int i, j, fail = 0;
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

    fail |= PolT2BSCmp(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
    return fail;
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