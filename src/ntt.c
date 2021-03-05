#include "ntt.h"

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "reduce.h"

// zeta^{br(1,2,3...)}*RmodM
const int32_t root_table[] = {
    846038,  370173,   -2016489, -1216365, 1843388,  -677362,  -1072953,
    -273335, 571552,   355329,   -1953862, 1203721,  1720831,  965995,
    641414,  1406204,  -869335,  -603157,  348730,   2063531,  -1328182,
    1381183, -1069471, 184133,   1465601,  -53304,   -1356023, 857228,
    59677,   675693,   -1598775, -136014,  966523,   959523,   846643,
    -86562,  -489847,  136654,   -2088895, 17941,    -1051723, -1316589,
    1814059, -230501,  1626667,  -1171284, 2085817,  1830521,  -1951522,
    445122,  -1689285, -1551600, -2055310, -1064338, -368446,  535845,
    361370,  676319,   -541241,  1009639,  538875,   -2102677, 1585701};

// zeta^{-i} in intt, montgomery field
const int32_t inv_root_table[] = {
    -1585701, 2102677,  -538875,  -1009639, 541241,   -676319,  -361370,
    -535845,  368446,   1064338,  2055310,  1551600,  1689285,  -445122,
    1951522,  -1830521, -2085817, 1171284,  -1626667, 230501,   -1814059,
    1316589,  1051723,  -17941,   2088895,  -136654,  489847,   86562,
    -846643,  -959523,  -966523,  136014,   1598775,  -675693,  -59677,
    -857228,  1356023,  53304,    -1465601, -184133,  1069471,  -1381183,
    1328182,  -2063531, -348730,  603157,   869335,   -1406204, -641414,
    -965995,  -1720831, -1203721, 1953862,  -355329,  -571552,  273335,
    1072953,  677362,   -1843388, 1216365,  2016489,  -370173,  -846038};

/**
 * Name: fqmul
 *
 * Description: Finite field mod q multiplication
 *
 */
int32_t fqmul(int32_t a, int32_t b)
{
    return montgomery_reduce((int64_t)a * b);
}

/*************************************************
 * Name:        ntt
 *
 * Description: Number-theoretic transform (NTT).
 *              input is in standard order, output is in bitreversed order
 *
 * Arguments:   - int32_t in/out[256]: pointer to input/output polynomial
 **************************************************/
void ntt(const uint16_t in[256], int32_t out[256])
{
    unsigned int len, start, j, k;
    int32_t t, zeta;

    k = 0;
    len = 128;
    zeta = root_table[k++];
    // a sepearate first layer for storing results to output polynomial
    for (j = 0; j < len; j++) {
        t = fqmul(zeta, (int16_t)in[j + len]);
        out[j + len] = (int32_t)(int16_t)in[j] - t;
        out[j] = (int32_t)(int16_t)in[j] + t;
    }
    // remaining five layers
    for (len = 64; len >= 4; len >>= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = root_table[k++];
            for (j = start; j < start + len; j++) {
                t = fqmul(zeta, out[j + len]);
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
void invntt(int32_t in[256], int32_t out[256])
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
        zeta = inv_root_table[k++];
        for (j = start; j < start + len; j++) {
            t = in[j];
            out[j] = t + in[j + len];
            out[j + len] = t - in[j + len];
            out[j + len] = fqmul(zeta, out[j + len]);
        }
    }
    // remaining five layers
    for (len = 8; len <= 128; len <<= 1) {
        for (start = 0; start < 256; start = j + len) {
            zeta = inv_root_table[k++];
            for (j = start; j < start + len; j++) {
                t = out[j];
                out[j] = t + out[j + len];
                out[j + len] = t - out[j + len];
                out[j + len] = fqmul(zeta, out[j + len]);
            }
        }
    }

    // multiply mont^2/64, reduce to centered representatives, get low 13 bits
    for (j = 0; j < 256; j++) {
        out[j] = fqmul(out[j], f);
        out[j] = barrett_reduce(out[j]);
    }
}

/*************************************************
 * Name:        basemul
 *
 * Description: Multiplication of polynomials in Zq[X]/(X^4-zeta)
 * used for multiplication of elements in Rq in NTT domain
 *
 * Arguments:   - int32_t r[4]: pointer to the output polynomial
 *              - const int32_t a[4]: pointer to the first polynomial
 *              - const int32_t b[4]: pointer to the second polynomial
 *              - int32_t zeta: integer defining the reduction polynomial
 **************************************************/
void basemul(int32_t r[4], const int32_t a[4], const int32_t b[4], int32_t zeta)
{
    int64_t t;
    // r0=a0b0+zeta*(a1b3+a2b2+a3b1)
    t = (int64_t)a[1] * b[3];
    t += (int64_t)a[2] * b[2];
    t += (int64_t)a[3] * b[1];
    r[0] = montgomery_reduce(t);
    r[0] = fqmul(r[0], zeta);
    r[0] += fqmul(a[0], b[0]);

    // r1=a0b1+a1b0+zeta*(a2b3+a3b2)
    t = (int64_t)a[2] * b[3];
    t += (int64_t)a[3] * b[2];
    r[1] = montgomery_reduce(t);
    r[1] = fqmul(r[1], zeta);
    t = (int64_t)a[0] * b[1];
    t += (int64_t)a[1] * b[0];
    r[1] += montgomery_reduce(t);

    // r2=a0b2+a1b1+a2b0+zeta*(a3b3)
    r[2] = fqmul(a[3], b[3]);
    r[2] = fqmul(r[2], zeta);
    t = (int64_t)a[0] * b[2];
    t += (int64_t)a[1] * b[1];
    t += (int64_t)a[2] * b[0];
    r[2] += montgomery_reduce(t);

    // r3=a0b3+a1b2+a2b1+a3b0
    t = (int64_t)a[0] * b[3];
    t += (int64_t)a[1] * b[2];
    t += (int64_t)a[2] * b[1];
    t += (int64_t)a[3] * b[0];
    r[3] = montgomery_reduce(t);
}