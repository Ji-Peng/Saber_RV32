#include "ntt.h"

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "reduce.h"

// zeta^{br(1,2,3...)}*RmodM
const int32_t rootTable[64] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, -4582610, -709618,  3211370,  -2422739, 2533938,
    -3724866, 4226394,  3515215,  -5214712, 4180628,  -4354273, 1961582,
    -1469009, 3661715,  5089826,  2896842,  1150913,  1250,     -771147,
    -3214001, -2216070, 2587567,  4635835,  1247022,  1133020,  1610224,
    -4652015, 4035904,  254135,   2640679,  1621924,  -2966437, -1300452,
    -3963361, 3815660,  -4635716, -4810532, 394299,   -3565801, 723646,
    1340759,  1171195,  4777770,  -495362,  -1032438, -4833797, 152199};

const int32_t rootTableMerged[64] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, -2683848, -4582610, 2896842,  1150913,  1250,     -771147,
    723028,   -709618,  3211370,  -3214001, -2216070, 2587567,  4635835,
    3724084,  -2422739, 2533938,  1247022,  1133020,  1610224,  -4652015,
    -5052843, -3724866, 4226394,  4035904,  254135,   2640679,  1621924,
    -199509,  3515215,  -5214712, -2966437, -1300452, -3963361, 3815660,
    -650362,  4180628,  -4354273, -4635716, -4810532, 394299,   -3565801,
    -2775101, 1961582,  -1469009, 723646,   1340759,  1171195,  4777770,
    -717683,  3661715,  5089826,  -495362,  -1032438, -4833797, 152199};

// zeta^{-i} in intt, montgomery field
const int32_t invRootTable[64] = {
    -152199,  4833797,  1032438,  495362,   -4777770, -1171195, -1340759,
    -723646,  3565801,  -394299,  4810532,  4635716,  -3815660, 3963361,
    1300452,  2966437,  -1621924, -2640679, -254135,  -4035904, 4652015,
    -1610224, -1133020, -1247022, -4635835, -2587567, 2216070,  3214001,
    771147,   -1250,    -1150913, -2896842, -5089826, -3661715, 1469009,
    -1961582, 4354273,  -4180628, 5214712,  -3515215, -4226394, 3724866,
    -2533938, 2422739,  -3211370, 709618,   4582610,  2683848,  717683,
    2775101,  650362,   199509,   5052843,  -3724084, -723028,  3450405,
    1927818,  5071803,  1672980,  -4859845, 4362766,  3836025,  280030};

const int32_t invRootTableMerged[64] = {
    -152199,  4833797,  1032438,  495362,   -5089826, -3661715, 717683,
    -4777770, -1171195, -1340759, -723646,  1469009,  -1961582, 2775101,
    3565801,  -394299,  4810532,  4635716,  4354273,  -4180628, 650362,
    -3815660, 3963361,  1300452,  2966437,  5214712,  -3515215, 199509,
    -1621924, -2640679, -254135,  -4035904, -4226394, 3724866,  5052843,
    4652015,  -1610224, -1133020, -1247022, -2533938, 2422739,  -3724084,
    -4635835, -2587567, 2216070,  3214001,  -3211370, 709618,   -723028,
    771147,   -1250,    -1150913, -2896842, 4582610,  2683848,  3450405,
    1927818,  5071803,  1672980,  -4859845, 4362766,  3836025,  280030};
/**
 * Name: FqMul
 *
 * Description: Finite field mod q multiplication
 *
 */
int32_t FqMul(int32_t a, int32_t b)
{
    return MontReduce((int64_t)a * b);
}

#define NTTASM

#ifdef NTTASM
extern void ntt_asm(const uint16_t in[SABER_N], int32_t out[SABER_N],
                    const int32_t rootTable[64]);
void NTT(const uint16_t in[SABER_N], int32_t out[SABER_N])
{
    ntt_asm(in, out, rootTableMerged);
}
#else
/*************************************************
 * Name:        NTT
 *
 * Description: Number-theoretic transform (NTT).
 * input is in standard order, output is in bitreversed order
 * input and output can not be same address because different data type
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void NTT(const uint16_t in[256], int32_t out[256])
{
    unsigned int len, start, j, k;
    int32_t t, zeta;

    k = 0;
    len = 128;
    zeta = rootTable[k++];
    // a sepearate first layer for storing results to output polynomial
    for (j = 0; j < len; j++) {
        t = FqMul(zeta, (int32_t)(int16_t)in[j + len]);
        out[j + len] = (int32_t)(int16_t)in[j] - t;
        out[j] = (int32_t)(int16_t)in[j] + t;
    }
    // remaining five layers
    for (len = 64; len >= 4; len >>= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = rootTable[k++];
            for (j = start; j < start + len; j++) {
                t = FqMul(zeta, out[j + len]);
                out[j + len] = out[j] - t;
                out[j] = out[j] + t;
            }
        }
    }
}
#endif

/*************************************************
 * Name:        invntt_tomont
 *
 * Description: Inverse number-theoretic transform and
 *              multiplication by Montgomery factor 2^32.
 *              Input is in bitreversed order, output is in standard order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void InvNTT(int32_t in[256], int32_t out[256])
{
    unsigned int start, len, j, k;
    int32_t t, zeta;
    // mont^2/64 mod M = (2^32)^2/64 mod M
    const int32_t f = NINV;
    // mont^2/64 mod M = (2^32)/64 mod M
    // const int32_t f = 4025329;

    k = 0;
    len = 4;
    // a separate first layer for storing results to output polynomial
    for (start = 0; start < 256; start = j + len) {
        zeta = invRootTable[k++];
        for (j = start; j < start + len; j++) {
            t = in[j];
            out[j] = t + in[j + len];
            out[j + len] = t - in[j + len];
            out[j + len] = FqMul(zeta, out[j + len]);
        }
    }
    // remaining five layers
    for (len = 8; len <= 128; len <<= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = invRootTable[k++];
            for (j = start; j < start + len; j++) {
                t = out[j];
                out[j] = t + out[j + len];
                out[j + len] = t - out[j + len];
                out[j + len] = FqMul(zeta, out[j + len]);
            }
        }
    }

    // multiply mont^2/64, reduce to centered representatives, get low 13 bits
    for (j = 0; j < 256; j++) {
        out[j] = FqMul(out[j], f);
        out[j] = BarrettReduce(out[j]);
    }
}

extern void intt_asm(uint32_t in[256], int32_t out[256],
                     const int32_t invRootTable[64]);
void InvNTTAsm(int32_t in[256], int32_t out[256])
{
    intt_asm(in, out, invRootTableMerged);
}

/*************************************************
 * Name:        BaseMul
 *
 * Description: Multiplication of polynomials in Zq[X]/(X^4-zeta)
 * used for multiplication of elements in Rq in NTT domain
 *
 * Arguments:   - int32_t a[4]: pointer to the first polynomial, is also output
 *              - const int32_t b[4]: pointer to the second polynomial
 *              - int32_t zeta: integer defining the reduction polynomial
 **************************************************/
void BaseMul(int32_t a[4], const int32_t b[4], int32_t zeta)
{
    int64_t t;
    int32_t a0, a1, a2, a3;

    // get values from memory for storing result
    a0 = a[0];
    a1 = a[1];
    a2 = a[2];
    a3 = a[3];

    // r0=a0b0+zeta*(a1b3+a2b2+a3b1)
    t = (int64_t)a1 * b[3];
    t += (int64_t)a2 * b[2];
    t += (int64_t)a3 * b[1];
    a[0] = MontReduce(t);
    a[0] = FqMul(a[0], zeta);
    a[0] += FqMul(a0, b[0]);

    // r1=a0b1+a1b0+zeta*(a2b3+a3b2)
    t = (int64_t)a2 * b[3];
    t += (int64_t)a3 * b[2];
    a[1] = MontReduce(t);
    a[1] = FqMul(a[1], zeta);
    t = (int64_t)a0 * b[1];
    t += (int64_t)a1 * b[0];
    a[1] += MontReduce(t);

    // r2=a0b2+a1b1+a2b0+zeta*(a3b3)
    a[2] = FqMul(a3, b[3]);
    a[2] = FqMul(a[2], zeta);
    t = (int64_t)a0 * b[2];
    t += (int64_t)a1 * b[1];
    t += (int64_t)a2 * b[0];
    a[2] += MontReduce(t);

    // r3=a0b3+a1b2+a2b1+a3b0
    t = (int64_t)a0 * b[3];
    t += (int64_t)a1 * b[2];
    t += (int64_t)a2 * b[1];
    t += (int64_t)a3 * b[0];
    a[3] = MontReduce(t);
}