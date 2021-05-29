#include "ntt.h"

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "reduce.h"

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
#if defined(FIVE_LAYER_NTT)
int32_t rootTable[32] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, -4582610, -709618,  3211370,  -2422739, 2533938,
    -3724866, 4226394,  3515215,  -5214712, 4180628,  -4354273, 1961582,
    -1469009, 3661715,  5089826};
// invRootTable[31]=280030*R*(1/32)=-2936516
int32_t invRootTable[32] = {
    -5089826, -3661715, 1469009, -1961582, 4354273, -4180628, 5214712,
    -3515215, -4226394, 3724866, -2533938, 2422739, -3211370, 709618,
    4582610,  2683848,  717683,  2775101,  650362,  199509,   5052843,
    -3724084, -723028,  3450405, 1927818,  5071803, 1672980,  -4859845,
    4362766,  3836025,  -2936516};

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
    // remaining seven layers
    for (len = 64; len >= 8; len >>= 1) {
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
    // mont^2/32 mod M = (2^32)^2/32 mod M
    const int32_t f = ((((int64_t)1 << 32) >> 5) * ((int64_t)1 << 32)) % M;

    k = 0;
    len = 8;
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
    // mod M for avoiding overflow
    // out[0] = FqMul(out[0], ((int64_t)1 << 32) % M);
    // remaining seven layers
    for (len = 16; len <= 128; len <<= 1) {
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

    // multiply mont^2/64, reduce to centered representatives
    for (j = 0; j < SABER_N / 2; j++) {
        out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }
    for (j = SABER_N / 2; j < SABER_N; j++) {
        out[j] = CenReduce(out[j]);
    }
}

/*************************************************
 * Name:        BaseMul
 *
 * Description: Multiplication of polynomials in Zq[X]/(X^4-zeta)
 * used for multiplication of elements in Rq in NTT domain
 *
 * Arguments:   - a: pointer to the first polynomial, is also output
 *              - b: pointer to the second polynomial
 *              - int32_t zeta: integer defining the reduction polynomial
 **************************************************/
void BaseMul(int32_t a[8], const int32_t b[8], int32_t zeta)
{
    int64_t t;
    int32_t a0, a1, a2, a3, a4, a5, a6, a7;
    a0 = a[0];
    a1 = a[1];
    a2 = a[2];
    a3 = a[3];
    a4 = a[4];
    a5 = a[5];
    a6 = a[6];
    a7 = a[7];

    t = (int64_t)a1 * b[7];
    t += (int64_t)a2 * b[6];
    t += (int64_t)a3 * b[5];
    t += (int64_t)a4 * b[4];
    t += (int64_t)a5 * b[3];
    t += (int64_t)a6 * b[2];
    t += (int64_t)a7 * b[1];
    a[0] = MontReduce(t);
    a[0] = FqMul(a[0], zeta);
    a[0] += FqMul(a0, b[0]);

    t = (int64_t)a2 * b[7];
    t += (int64_t)a3 * b[6];
    t += (int64_t)a4 * b[5];
    t += (int64_t)a5 * b[4];
    t += (int64_t)a6 * b[3];
    t += (int64_t)a7 * b[2];
    a[1] = MontReduce(t);
    a[1] = FqMul(a[1], zeta);
    t = (int64_t)a0 * b[1];
    t += (int64_t)a1 * b[0];
    a[1] += MontReduce(t);

    t = (int64_t)a3 * b[7];
    t += (int64_t)a4 * b[6];
    t += (int64_t)a5 * b[5];
    t += (int64_t)a6 * b[4];
    t += (int64_t)a7 * b[3];
    a[2] = MontReduce(t);
    a[2] = FqMul(a[2], zeta);
    t = (int64_t)a0 * b[2];
    t += (int64_t)a1 * b[1];
    t += (int64_t)a2 * b[0];
    a[2] += MontReduce(t);

    t = (int64_t)a4 * b[7];
    t += (int64_t)a5 * b[6];
    t += (int64_t)a6 * b[5];
    t += (int64_t)a7 * b[4];
    a[3] = MontReduce(t);
    a[3] = FqMul(a[3], zeta);
    t = (int64_t)a0 * b[3];
    t += (int64_t)a1 * b[2];
    t += (int64_t)a2 * b[1];
    t += (int64_t)a3 * b[0];
    a[3] += MontReduce(t);

    t = (int64_t)a5 * b[7];
    t += (int64_t)a6 * b[6];
    t += (int64_t)a7 * b[5];
    a[4] = MontReduce(t);
    a[4] = FqMul(a[4], zeta);
    t = (int64_t)a0 * b[4];
    t += (int64_t)a1 * b[3];
    t += (int64_t)a2 * b[2];
    t += (int64_t)a3 * b[1];
    t += (int64_t)a4 * b[0];
    a[4] += MontReduce(t);

    t = (int64_t)a6 * b[7];
    t += (int64_t)a7 * b[6];
    a[5] = MontReduce(t);
    a[5] = FqMul(a[5], zeta);
    t = (int64_t)a0 * b[5];
    t += (int64_t)a1 * b[4];
    t += (int64_t)a2 * b[3];
    t += (int64_t)a3 * b[2];
    t += (int64_t)a4 * b[1];
    t += (int64_t)a5 * b[0];
    a[5] += MontReduce(t);

    a[6] = FqMul(a7, b[7]);
    a[6] = FqMul(a[6], zeta);
    t = (int64_t)a0 * b[6];
    t += (int64_t)a1 * b[5];
    t += (int64_t)a2 * b[4];
    t += (int64_t)a3 * b[3];
    t += (int64_t)a4 * b[2];
    t += (int64_t)a5 * b[1];
    t += (int64_t)a6 * b[0];
    a[6] += MontReduce(t);

    t = (int64_t)a0 * b[7];
    t += (int64_t)a1 * b[6];
    t += (int64_t)a2 * b[5];
    t += (int64_t)a3 * b[4];
    t += (int64_t)a4 * b[3];
    t += (int64_t)a5 * b[2];
    t += (int64_t)a6 * b[1];
    t += (int64_t)a7 * b[0];
    a[7] = MontReduce(t);
}
#elif defined(SIX_LAYER_NTT)
#    ifdef NTTASM
int32_t rootTableMerged[] = {
    -280030,     -3836025,    -4362766,    4859845,     -1672980,
    -5071803,    -1927818,    -3450405,    -2683848,    -4582610,
    2896842,     1150913,     1250,        -771147,     723028,
    -709618,     3211370,     -3214001,    -2216070,    2587567,
    4635835,     3724084,     -2422739,    2533938,     1247022,
    1133020,     1610224,     -4652015,    -5052843,    -3724866,
    4226394,     4035904,     254135,      2640679,     1621924,
    -199509,     3515215,     -5214712,    -2966437,    -1300452,
    -3963361,    3815660,     -650362,     4180628,     -4354273,
    -4635716,    -4810532,    394299,      -3565801,    -2775101,
    1961582,     -1469009,    723646,      1340759,     1171195,
    4777770,     -717683,     3661715,     5089826,     -495362,
    -1032438,    -4833797,    152199};
// invRootTableMerged[63]=280030*R*(1/64)=-1468258
int32_t invRootTableMerged[] = {
    -152199,     4833797,     1032438,     495362,      -5089826,
    -3661715,    717683,      -4777770,    -1171195,    -1340759,
    -723646,     1469009,     -1961582,    2775101,     3565801,
    -394299,     4810532,     4635716,     4354273,     -4180628,
    650362,      -3815660,    3963361,     1300452,     2966437,
    5214712,     -3515215,    199509,      -1621924,    -2640679,
    -254135,     -4035904,    -4226394,    3724866,     5052843,
    4652015,     -1610224,    -1133020,    -1247022,    -2533938,
    2422739,     -3724084,    -4635835,    -2587567,    2216070,
    3214001,     -3211370,    709618,      -723028,     771147,
    -1250,       -1150913,    -2896842,    4582610,     2683848,
    3450405,     1927818,     5071803,     1672980,     -4859845,
    4362766,     3836025,     -1468258};
extern void ntt_asm(const uint16_t in[SABER_N], int32_t out[SABER_N],
                    int32_t rootTableMerged[SABER_N / 4]);
extern void intt_asm(int32_t in[SABER_N], int32_t out[SABER_N],
                     int32_t invRootTableMerged[SABER_N / 4]);
extern void basemul_asm(int32_t a[4], const int32_t b[4], int32_t zeta);

void NTT(const uint16_t in[SABER_N], int32_t out[SABER_N])
{
    ntt_asm(in, out, rootTableMerged);
}

void InvNTT(int32_t in[SABER_N], int32_t out[SABER_N])
{
    intt_asm(in, out, invRootTableMerged);
}

void BaseMul(int32_t a[4], const int32_t b[4], int32_t zeta)
{
    basemul_asm(a, b, zeta);
}
#    else

// zeta^{br(1,2,3...)}*RmodM
int32_t rootTable[64] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, -4582610, -709618,  3211370,  -2422739, 2533938,
    -3724866, 4226394,  3515215,  -5214712, 4180628,  -4354273, 1961582,
    -1469009, 3661715,  5089826,  2896842,  1150913,  1250,     -771147,
    -3214001, -2216070, 2587567,  4635835,  1247022,  1133020,  1610224,
    -4652015, 4035904,  254135,   2640679,  1621924,  -2966437, -1300452,
    -3963361, 3815660,  -4635716, -4810532, 394299,   -3565801, 723646,
    1340759,  1171195,  4777770,  -495362,  -1032438, -4833797, 152199};

// zeta^{-i} in intt, montgomery field
// invRootTable[63]=280030*R*(1/64)=-1468258
int32_t invRootTable[64] = {
    -152199,  4833797,  1032438,  495362,   -4777770, -1171195, -1340759,
    -723646,  3565801,  -394299,  4810532,  4635716,  -3815660, 3963361,
    1300452,  2966437,  -1621924, -2640679, -254135,  -4035904, 4652015,
    -1610224, -1133020, -1247022, -4635835, -2587567, 2216070,  3214001,
    771147,   -1250,    -1150913, -2896842, -5089826, -3661715, 1469009,
    -1961582, 4354273,  -4180628, 5214712,  -3515215, -4226394, 3724866,
    -2533938, 2422739,  -3211370, 709618,   4582610,  2683848,  717683,
    2775101,  650362,   199509,   5052843,  -3724084, -723028,  3450405,
    1927818,  5071803,  1672980,  -4859845, 4362766,  3836025,  -1468258};

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
    const int32_t f =
        (((((int64_t)1 << 32) >> 6) % M) * (((int64_t)1 << 32) % M)) % M;

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
    for (j = 0; j < SABER_N / 2; j++) {
        out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }
    for (j = SABER_N / 2; j < SABER_N; j++) {
        // out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }
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
#    endif

#elif defined(SEVEN_LAYER_NTT)
#    ifdef NTTASM
#    else
int32_t rootTable[] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, -4582610, -709618,  3211370,  -2422739, 2533938,
    -3724866, 4226394,  3515215,  -5214712, 4180628,  -4354273, 1961582,
    -1469009, 3661715,  5089826,  2896842,  1150913,  1250,     -771147,
    -3214001, -2216070, 2587567,  4635835,  1247022,  1133020,  1610224,
    -4652015, 4035904,  254135,   2640679,  1621924,  -2966437, -1300452,
    -3963361, 3815660,  -4635716, -4810532, 394299,   -3565801, 723646,
    1340759,  1171195,  4777770,  -495362,  -1032438, -4833797, 152199,
    -4787907, 4312107,  -563551,  -2485656, -4934765, -1274402, 4806806,
    -2404203, 844313,   -5047542, -315757,  1140755,  -913766,  917326,
    -1367707, -5171082, 883183,   2100688,  -3038201, -4533557, 1436016,
    -4273149, 3975068,  5163653,  4695769,  2573915,  -653503,  434027,
    4192924,  -881546,  -2887756, 2791875,  -909077,  -4601548, 239548,
    -3183857, 5136869,  3554108,  -5221924, 128531,   -4205624, -4707996,
    2940826,  4825397,  -3821535, -1387751, 1063300,  -4465789, -3282628,
    -2174102, -4896547, 1107666,  -3193337, 658585,   3591289,  -2710765,
    2655674,  -622899,  3866337,  -4722017, -842149,  4652240,  1066461,
    -1272644};

// invRootTable[126]=280030*R*(1/128)=-734129
int32_t invRootTable[] = {
    1272644,  -1066461, -4652240, 842149,   4722017,  -3866337, 622899,
    -2655674, 2710765,  -3591289, -658585,  3193337,  -1107666, 4896547,
    2174102,  3282628,  4465789,  -1063300, 1387751,  3821535,  -4825397,
    -2940826, 4707996,  4205624,  -128531,  5221924,  -3554108, -5136869,
    3183857,  -239548,  4601548,  909077,   -2791875, 2887756,  881546,
    -4192924, -434027,  653503,   -2573915, -4695769, -5163653, -3975068,
    4273149,  -1436016, 4533557,  3038201,  -2100688, -883183,  5171082,
    1367707,  -917326,  913766,   -1140755, 315757,   5047542,  -844313,
    2404203,  -4806806, 1274402,  4934765,  2485656,  563551,   -4312107,
    4787907,  -152199,  4833797,  1032438,  495362,   -4777770, -1171195,
    -1340759, -723646,  3565801,  -394299,  4810532,  4635716,  -3815660,
    3963361,  1300452,  2966437,  -1621924, -2640679, -254135,  -4035904,
    4652015,  -1610224, -1133020, -1247022, -4635835, -2587567, 2216070,
    3214001,  771147,   -1250,    -1150913, -2896842, -5089826, -3661715,
    1469009,  -1961582, 4354273,  -4180628, 5214712,  -3515215, -4226394,
    3724866,  -2533938, 2422739,  -3211370, 709618,   4582610,  2683848,
    717683,   2775101,  650362,   199509,   5052843,  -3724084, -723028,
    3450405,  1927818,  5071803,  1672980,  -4859845, 4362766,  3836025,
    -734129};

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
    for (len = 64; len >= 2; len >>= 1) {
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
    // mont^2/128 mod M = (2^32)^2/128 mod M
    const int32_t f = ((((int64_t)1 << 32) >> 7) * ((int64_t)1 << 32)) % M;

    k = 0;
    len = 2;
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
    for (len = 4; len <= 128; len <<= 1) {
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
    for (j = 0; j < SABER_N / 2; j++) {
        out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }

    for (j = SABER_N / 2; j < SABER_N; j++) {
        // out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }
}

/*************************************************
 * Name:        BaseMul
 *
 * Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
 * used for multiplication of elements in Rq in NTT domain
 *
 * Arguments:   - int32_t a[2]: pointer to the first polynomial, is also output
 *              - const int32_t b[2]: pointer to the second polynomial
 *              - int32_t zeta: integer defining the reduction polynomial
 **************************************************/
void BaseMul(int32_t a[2], const int32_t b[2], int32_t zeta)
{
    int64_t t;
    int32_t a0, a1;

    // get values from memory for storing result
    a0 = a[0];
    a1 = a[1];

    // r0=a0b0+zeta*(a1b1)
    t = FqMul(a1, b[1]);
    t = FqMul(t, zeta);
    a[0] = t + FqMul(a0, b[0]);

    // r1=a0b1+a1b0
    t = (int64_t)a0 * b[1];
    t += (int64_t)a1 * b[0];
    a[1] = MontReduce(t);
}
#    endif
#elif defined(COMPLETE_NTT)
#    ifdef NTTASM
int32_t rootTableMerged[] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, 2896842,  1150913,  -4787907, 4312107,  -563551,
    -2485656, 3481834,  3858805,  4212763,  4809384,  3364370,  2809068,
    -1176165, -1795592, -4582610, 1250,     -771147,  -4934765, -1274402,
    4806806,  -2404203, -4596506, -2318885, -1008267, 1886097,  -238541,
    3242231,  -1175084, 1506107,  -709618,  -3214001, -2216070, 844313,
    -5047542, -315757,  1140755,  4572371,  805258,   1398155,  2613913,
    -1436330, 1849104,  3689777,  3685793,  3211370,  2587567,  4635835,
    -913766,  917326,   -1367707, -5171082, 29341,    1221760,  2792507,
    4226255,  5084933,  2553547,  -2263202, 3293867,  -2422739, 1247022,
    1133020,  883183,   2100688,  -3038201, -4533557, -3500010, 2253019,
    -2220927, -1181438, -1236673, -1501693, -2091150, -3708879, 2533938,
    1610224,  -4652015, 1436016,  -4273149, 3975068,  5163653,  -4251970,
    3176537,  -2629672, 2755083,  4681558,  -1353513, 2849516,  -1602906,
    -3724866, 4035904,  254135,   4695769,  2573915,  -653503,  434027,
    1004975,  2359821,  3852135,  1807641,  -3995044, -3395038, -5041114,
    1684192,  4226394,  2640679,  1621924,  4192924,  -881546,  -2887756,
    2791875,  1650798,  -3320673, 4778722,  2570867,  530317,   569555,
    -2290380, 5176155,  3515215,  -2966437, -1300452, -909077,  -4601548,
    239548,   -3183857, 1503762,  -4803613, -536181,  -5023368, -4578747,
    3480599,  131466,   2194685,  -5214712, -3963361, 3815660,  5136869,
    3554108,  -5221924, 128531,   3313975,  43983,    -3401722, -4651435,
    -2487062, 1867230,  2167701,  4584507,  4180628,  -4635716, -4810532,
    -4205624, -4707996, 2940826,  4825397,  199372,   -2377902, 207399,
    5179959,  1706077,  3747682,  -2661611, -85780,   -4354273, 394299,
    -3565801, -3821535, -1387751, 1063300,  -4465789, 3508520,  3656041,
    3176340,  1428526,  -3213521, -3854630, -4186095, -4950702, 1961582,
    723646,   1340759,  -3282628, -2174102, -4896547, 1107666,  2329314,
    -1742336, 3495906,  1268860,  2597864,  -2547200, 309176,   -3146969,
    -1469009, 1171195,  4777770,  -3193337, 658585,   3591289,  -2710765,
    -231627,  4111694,  -2343596, 5049151,  -2905874, 5209770,  -2242748,
    -3602417, 3661715,  -495362,  -1032438, 2655674,  -622899,  3866337,
    -4722017, -2061934, 4931462,  -1970620, -2637635, -262271,  346069,
    -3223033, 4144613,  5089826,  -4833797, 152199,   -842149,  4652240,
    1066461,  -1272644, -2942960, -4532505, 2222962,  1226499,  -5128404,
    2508567,  -1708977, 2656015};

// invRootTableMerged[254]=280030*R*(1/256) mod M=4876840
int32_t invRootTableMerged[] = {
    -2656015, 1708977,  -2508567, 5128404,  -1226499, -2222962, 4532505,
    2942960,  1272644,  -1066461, -4652240, 842149,   -152199,  4833797,
    -5089826, -4144613, 3223033,  -346069,  262271,   2637635,  1970620,
    -4931462, 2061934,  4722017,  -3866337, 622899,   -2655674, 1032438,
    495362,   -3661715, 3602417,  2242748,  -5209770, 2905874,  -5049151,
    2343596,  -4111694, 231627,   2710765,  -3591289, -658585,  3193337,
    -4777770, -1171195, 1469009,  3146969,  -309176,  2547200,  -2597864,
    -1268860, -3495906, 1742336,  -2329314, -1107666, 4896547,  2174102,
    3282628,  -1340759, -723646,  -1961582, 4950702,  4186095,  3854630,
    3213521,  -1428526, -3176340, -3656041, -3508520, 4465789,  -1063300,
    1387751,  3821535,  3565801,  -394299,  4354273,  85780,    2661611,
    -3747682, -1706077, -5179959, -207399,  2377902,  -199372,  -4825397,
    -2940826, 4707996,  4205624,  4810532,  4635716,  -4180628, -4584507,
    -2167701, -1867230, 2487062,  4651435,  3401722,  -43983,   -3313975,
    -128531,  5221924,  -3554108, -5136869, -3815660, 3963361,  5214712,
    -2194685, -131466,  -3480599, 4578747,  5023368,  536181,   4803613,
    -1503762, 3183857,  -239548,  4601548,  909077,   1300452,  2966437,
    -3515215, -5176155, 2290380,  -569555,  -530317,  -2570867, -4778722,
    3320673,  -1650798, -2791875, 2887756,  881546,   -4192924, -1621924,
    -2640679, -4226394, -1684192, 5041114,  3395038,  3995044,  -1807641,
    -3852135, -2359821, -1004975, -434027,  653503,   -2573915, -4695769,
    -254135,  -4035904, 3724866,  1602906,  -2849516, 1353513,  -4681558,
    -2755083, 2629672,  -3176537, 4251970,  -5163653, -3975068, 4273149,
    -1436016, 4652015,  -1610224, -2533938, 3708879,  2091150,  1501693,
    1236673,  1181438,  2220927,  -2253019, 3500010,  4533557,  3038201,
    -2100688, -883183,  -1133020, -1247022, 2422739,  -3293867, 2263202,
    -2553547, -5084933, -4226255, -2792507, -1221760, -29341,   5171082,
    1367707,  -917326,  913766,   -4635835, -2587567, -3211370, -3685793,
    -3689777, -1849104, 1436330,  -2613913, -1398155, -805258,  -4572371,
    -1140755, 315757,   5047542,  -844313,  2216070,  3214001,  709618,
    -1506107, 1175084,  -3242231, 238541,   -1886097, 1008267,  2318885,
    4596506,  2404203,  -4806806, 1274402,  4934765,  771147,   -1250,
    4582610,  1795592,  1176165,  -2809068, -3364370, -4809384, -4212763,
    -3858805, -3481834, 2485656,  563551,   -4312107, 4787907,  -1150913,
    -2896842, 2683848,  717683,   2775101,  650362,   199509,   5052843,
    -3724084, -723028,  3450405,  1927818,  5071803,  1672980,  -4859845,
    4362766,  3836025,  4876840};

extern void ntt_asm_8layer(const uint16_t in[SABER_N], int32_t out[SABER_N],
                           int32_t *rootTableMerged);

extern void intt_asm_8layer(const int32_t in[SABER_N], int32_t out[SABER_N],
                            int32_t *invRootTableMerged);
extern void basemul_asm_8layer(int32_t a[SABER_N], const int32_t b[SABER_N]);

void NTT(const uint16_t in[SABER_N], int32_t out[SABER_N])
{
    ntt_asm_8layer(in, out, rootTableMerged);
}

void InvNTT(int32_t in[SABER_N], int32_t out[SABER_N])
{
    intt_asm_8layer(in, out, invRootTableMerged);
}

void BaseMul(int32_t a[SABER_N], const int32_t b[SABER_N])
{
    basemul_asm_8layer(a, b);
}
#    else
int32_t rootTable[] = {
    -280030,  -3836025, -4362766, 4859845,  -1672980, -5071803, -1927818,
    -3450405, 723028,   3724084,  -5052843, -199509,  -650362,  -2775101,
    -717683,  -2683848, -4582610, -709618,  3211370,  -2422739, 2533938,
    -3724866, 4226394,  3515215,  -5214712, 4180628,  -4354273, 1961582,
    -1469009, 3661715,  5089826,  2896842,  1150913,  1250,     -771147,
    -3214001, -2216070, 2587567,  4635835,  1247022,  1133020,  1610224,
    -4652015, 4035904,  254135,   2640679,  1621924,  -2966437, -1300452,
    -3963361, 3815660,  -4635716, -4810532, 394299,   -3565801, 723646,
    1340759,  1171195,  4777770,  -495362,  -1032438, -4833797, 152199,
    -4787907, 4312107,  -563551,  -2485656, -4934765, -1274402, 4806806,
    -2404203, 844313,   -5047542, -315757,  1140755,  -913766,  917326,
    -1367707, -5171082, 883183,   2100688,  -3038201, -4533557, 1436016,
    -4273149, 3975068,  5163653,  4695769,  2573915,  -653503,  434027,
    4192924,  -881546,  -2887756, 2791875,  -909077,  -4601548, 239548,
    -3183857, 5136869,  3554108,  -5221924, 128531,   -4205624, -4707996,
    2940826,  4825397,  -3821535, -1387751, 1063300,  -4465789, -3282628,
    -2174102, -4896547, 1107666,  -3193337, 658585,   3591289,  -2710765,
    2655674,  -622899,  3866337,  -4722017, -842149,  4652240,  1066461,
    -1272644, 3481834,  3858805,  4212763,  4809384,  3364370,  2809068,
    -1176165, -1795592, -4596506, -2318885, -1008267, 1886097,  -238541,
    3242231,  -1175084, 1506107,  4572371,  805258,   1398155,  2613913,
    -1436330, 1849104,  3689777,  3685793,  29341,    1221760,  2792507,
    4226255,  5084933,  2553547,  -2263202, 3293867,  -3500010, 2253019,
    -2220927, -1181438, -1236673, -1501693, -2091150, -3708879, -4251970,
    3176537,  -2629672, 2755083,  4681558,  -1353513, 2849516,  -1602906,
    1004975,  2359821,  3852135,  1807641,  -3995044, -3395038, -5041114,
    1684192,  1650798,  -3320673, 4778722,  2570867,  530317,   569555,
    -2290380, 5176155,  1503762,  -4803613, -536181,  -5023368, -4578747,
    3480599,  131466,   2194685,  3313975,  43983,    -3401722, -4651435,
    -2487062, 1867230,  2167701,  4584507,  199372,   -2377902, 207399,
    5179959,  1706077,  3747682,  -2661611, -85780,   3508520,  3656041,
    3176340,  1428526,  -3213521, -3854630, -4186095, -4950702, 2329314,
    -1742336, 3495906,  1268860,  2597864,  -2547200, 309176,   -3146969,
    -231627,  4111694,  -2343596, 5049151,  -2905874, 5209770,  -2242748,
    -3602417, -2061934, 4931462,  -1970620, -2637635, -262271,  346069,
    -3223033, 4144613,  -2942960, -4532505, 2222962,  1226499,  -5128404,
    2508567,  -1708977, 2656015};

// invRootTable[254]=280030*R*(1/256) mod M=4876840
int32_t invRootTable[] = {
    -2656015, 1708977,  -2508567, 5128404,  -1226499, -2222962, 4532505,
    2942960,  -4144613, 3223033,  -346069,  262271,   2637635,  1970620,
    -4931462, 2061934,  3602417,  2242748,  -5209770, 2905874,  -5049151,
    2343596,  -4111694, 231627,   3146969,  -309176,  2547200,  -2597864,
    -1268860, -3495906, 1742336,  -2329314, 4950702,  4186095,  3854630,
    3213521,  -1428526, -3176340, -3656041, -3508520, 85780,    2661611,
    -3747682, -1706077, -5179959, -207399,  2377902,  -199372,  -4584507,
    -2167701, -1867230, 2487062,  4651435,  3401722,  -43983,   -3313975,
    -2194685, -131466,  -3480599, 4578747,  5023368,  536181,   4803613,
    -1503762, -5176155, 2290380,  -569555,  -530317,  -2570867, -4778722,
    3320673,  -1650798, -1684192, 5041114,  3395038,  3995044,  -1807641,
    -3852135, -2359821, -1004975, 1602906,  -2849516, 1353513,  -4681558,
    -2755083, 2629672,  -3176537, 4251970,  3708879,  2091150,  1501693,
    1236673,  1181438,  2220927,  -2253019, 3500010,  -3293867, 2263202,
    -2553547, -5084933, -4226255, -2792507, -1221760, -29341,   -3685793,
    -3689777, -1849104, 1436330,  -2613913, -1398155, -805258,  -4572371,
    -1506107, 1175084,  -3242231, 238541,   -1886097, 1008267,  2318885,
    4596506,  1795592,  1176165,  -2809068, -3364370, -4809384, -4212763,
    -3858805, -3481834, 1272644,  -1066461, -4652240, 842149,   4722017,
    -3866337, 622899,   -2655674, 2710765,  -3591289, -658585,  3193337,
    -1107666, 4896547,  2174102,  3282628,  4465789,  -1063300, 1387751,
    3821535,  -4825397, -2940826, 4707996,  4205624,  -128531,  5221924,
    -3554108, -5136869, 3183857,  -239548,  4601548,  909077,   -2791875,
    2887756,  881546,   -4192924, -434027,  653503,   -2573915, -4695769,
    -5163653, -3975068, 4273149,  -1436016, 4533557,  3038201,  -2100688,
    -883183,  5171082,  1367707,  -917326,  913766,   -1140755, 315757,
    5047542,  -844313,  2404203,  -4806806, 1274402,  4934765,  2485656,
    563551,   -4312107, 4787907,  -152199,  4833797,  1032438,  495362,
    -4777770, -1171195, -1340759, -723646,  3565801,  -394299,  4810532,
    4635716,  -3815660, 3963361,  1300452,  2966437,  -1621924, -2640679,
    -254135,  -4035904, 4652015,  -1610224, -1133020, -1247022, -4635835,
    -2587567, 2216070,  3214001,  771147,   -1250,    -1150913, -2896842,
    -5089826, -3661715, 1469009,  -1961582, 4354273,  -4180628, 5214712,
    -3515215, -4226394, 3724866,  -2533938, 2422739,  -3211370, 709618,
    4582610,  2683848,  717683,   2775101,  650362,   199509,   5052843,
    -3724084, -723028,  3450405,  1927818,  5071803,  1672980,  -4859845,
    4362766,  3836025,  4876840};

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
    // remaining seven layers
    for (len = 64; len >= 1; len >>= 1) {
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
    // mont^2/256 mod M = (2^32)^2/256 mod M
    const int32_t f = ((((int64_t)1 << 32) >> 8) * ((int64_t)1 << 32)) % M;

    k = 0;
    len = 1;
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
    // mod M for avoiding overflow
    out[0] = FqMul(out[0], ((int64_t)1 << 32) % M);
    // remaining seven layers
    for (len = 2; len <= 128; len <<= 1) {
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

    // multiply mont^2/64, reduce to centered representatives
    for (j = 0; j < SABER_N / 2; j++) {
        out[j] = FqMul(out[j], f);
        out[j] = CenReduce(out[j]);
    }
    for (j = SABER_N / 2; j < SABER_N; j++) {
        out[j] = CenReduce(out[j]);
    }
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
void BaseMul(int32_t a[SABER_N], const int32_t b[SABER_N])
{
    int32_t i;
    for (i = 0; i < SABER_N; i++) {
        a[i] = FqMul(a[i], b[i]);
    }
}
#    endif
#endif