#include "ntt.h"

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "reduce.h"

// zeta^{br(1,2,3...)}*RmodM
const int32_t rootTable[64] = {
    -2213763, 2428476,  1544621, -2259749, 1626771,  -1469218, -1091946,
    -74393,   642017,   2457939, -2395521, -1812081, -663400,  2594527,
    973024,   -1697208, -951560, -2235126, 2602746,  426661,   757074,
    283525,   1773148,  1421065, -1041766, -2612916, -1752798, 1951685,
    -825507,  -1168951, 573932,  140931,   -1143507, 2545466,  -338076,
    2257958,  388481,   1139189, -220382,  2454064,  -2072538, -1067428,
    1939754,  1827103,  266490,  -2604736, -2326354, 1089295,  401214,
    1631490,  -1091532, -7545,   -2611876, 1306028,  803989,   -2525856,
    104067,   2318338,  1654618, 572692,   1691777,  1696007,  -1933349};

// zeta^{-i} in intt, montgomery field
const int32_t invRootTable[64] = {
    1933349,  -1696007, -1691777, -572692, -1654618, -2318338, -104067,
    2525856,  -803989,  -1306028, 2611876, 7545,     1091532,  -1631490,
    -401214,  -1089295, 2326354,  2604736, -266490,  -1827103, -1939754,
    1067428,  2072538,  -2454064, 220382,  -1139189, -388481,  -2257958,
    338076,   -2545466, 1143507,  -140931, -573932,  1168951,  825507,
    -1951685, 1752798,  2612916,  1041766, -1421065, -1773148, -283525,
    -757074,  -426661,  -2602746, 2235126, 951560,   1697208,  -973024,
    -2594527, 663400,   1812081,  2395521, -2457939, -642017,  74393,
    1091946,  1469218,  -1626771, 2259749, -1544621, -2428476, 2213763};

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