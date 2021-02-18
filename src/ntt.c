#include "ntt.h"

#include <stdint.h>

#include "api.h"
#include "reduce.h"

// zeta^{br(1,2,3...)}*RmodM
const int32_t root_table[63] = {
    2921358,   -10203707, -1203107,  6577444,  1776511,  -4194664,  5735629,
    7301157,   -9084979,  -11637995, -7261676, 4034819,  647681,    2695651,
    -2559945,  -4359117,  5669200,   -7944926, 1686897,  -4810496,  7146164,
    -4293923,  -6267356,  -1988985,  -8060830, -9344183, 2733537,   11450840,
    -12030083, -908786,   -2665284,  -9600669, 10575964, 8064557,   -819256,
    -588496,   -8693794,  -7460755,  2723061,  4092287,  -3261033,  -5563113,
    -11307548, -9567042,  11980428,  6931502,  2510833,  -10319196, -6726360,
    10171507,  8693725,   -42688,    10505644, -9502337, 10910265,  5318976,
    -1134236,  -614272,   -6236460,  5184115,  -1069349, -9233574,  12174351};

// zeta^{-i} in intt, montgomery field
const int32_t inv_root_table[63] = {
    -12174351, 9233574,   1069349,   -5184115,  6236460,   614272,   1134236,
    -5318976,  -10910265, 9502337,   -10505644, 42688,     -8693725, -10171507,
    6726360,   10319196,  -2510833,  -6931502,  -11980428, 9567042,  11307548,
    5563113,   3261033,   -4092287,  -2723061,  7460755,   8693794,  588496,
    819256,    -8064557,  -10575964, 9600669,   2665284,   908786,   12030083,
    -11450840, -2733537,  9344183,   8060830,   1988985,   6267356,  4293923,
    -7146164,  4810496,   -1686897,  7944926,   -5669200,  4359117,  2559945,
    -2695651,  -647681,   -4034819,  7261676,   11637995,  9084979,  -7301157,
    -5735629,  4194664,   -1776511,  -6577444,  1203107,   10203707, -2921358};

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
void ntt(const int16_t in[256], int32_t out[256])
{
    unsigned int len, start, j, k;
    int32_t t, zeta;

    k = 0;
    len = 128;
    zeta = root_table[k++];
    // a sepearate first layer for storing results to output polynomial
    for (j = 0; j < len; j++) {
        t = fqmul(zeta, (int32_t)in[j + len]);
        out[j + len] = in[j] - t;
        out[j] = in[j] + t;
    }
    // remaining five layers
    for (len = 64; len >= 4; len >>= 1) {
        // printf("len is %d ", len);
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
    const int32_t f = 7689784;
    // mont/64 mod M = (2^32)/64 mod M
    // const int32_t f = 16776702;

    k = 0;
    len = 4;
    // a separate first layer for storing results to output polynomial
    for (start = 0; start < 256; start = j + len) {
        zeta = inv_root_table[k++];
        for (j = start; j < start + len; j++) {
            t = in[j];
            out[j] = barrett_reduce(t + in[j + len]);
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
                out[j] = barrett_reduce(t + out[j + len]);
                out[j + len] = t - out[j + len];
                out[j + len] = fqmul(zeta, out[j + len]);
            }
        }
    }

    // multiply mont^2/64, reduce to centered representatives, get low 13 bits
    for (j = 0; j < 256; j++) {
        out[j] = fqmul(out[j], f);
        out[j] = barrett_reduce(out[j]);
        out[j] &= 0x1fff;
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
    // r0=a0b0+zeta*(a1b3+a2b2+a3b1)
    r[0] = fqmul(a[1], b[3]);
    r[0] += fqmul(a[2], b[2]);
    r[0] += fqmul(a[3], b[1]);
    r[0] = fqmul(r[0], zeta);
    r[0] += fqmul(a[0], b[0]);
    // r1=a0b1+a1b0+zeta*(a2b3+a3b2)
    r[1] = fqmul(a[2], b[3]);
    r[1] += fqmul(a[3], b[2]);
    r[1] = fqmul(r[1], zeta);
    r[1] += fqmul(a[0], b[1]);
    r[1] += fqmul(a[1], b[0]);
    // r2=a0b2+a1b1+a2b0+zeta*(a3b3)
    r[2] = fqmul(a[3], b[3]);
    r[2] = fqmul(r[2], zeta);
    r[2] += fqmul(a[0], b[2]);
    r[2] += fqmul(a[1], b[1]);
    r[2] += fqmul(a[2], b[0]);
    // r3=a0b3+a1b2+a2b1+a3b0
    r[3] = fqmul(a[0], b[3]);
    r[3] += fqmul(a[1], b[2]);
    r[3] += fqmul(a[2], b[1]);
    r[3] += fqmul(a[3], b[0]);
}