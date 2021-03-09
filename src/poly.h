#ifndef POLY_H
#define POLY_H

#include <stdint.h>

#include "SABER_params.h"

void MatrixVectorMul(const uint16_t a[SABER_L][SABER_L][SABER_N],
                     const uint16_t s[SABER_L][SABER_N],
                     uint16_t res[SABER_L][SABER_N], int16_t transpose);
void InnerProd(const uint16_t b[SABER_L][SABER_N],
               const uint16_t s[SABER_L][SABER_N], uint16_t res[SABER_N]);
void GenMatrix(uint16_t a[SABER_L][SABER_L][SABER_N],
               const uint8_t seed[SABER_SEEDBYTES]);
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             uint8_t init, uint8_t nblocks);
void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES]);

void poly_basemul(int32_t a[SABER_N], const int32_t b[SABER_N]);

void poly_add(uint16_t res[SABER_N], int32_t in[SABER_N]);

void poly_mul_acc_ntt(const uint16_t a[SABER_N], const uint16_t b[SABER_N],
                      uint16_t res[SABER_N]);

void InnerProd_ntt(const int16_t b[SABER_L][SABER_N],
                   const int16_t s[SABER_L][SABER_N], int16_t res[SABER_N]);
void InnerProdInTime_ntt(const uint8_t *bytes,
                         const uint16_t s[SABER_L][SABER_N],
                         uint16_t res[SABER_N]);

void MatrixVectorMul_ntt(const int16_t A[SABER_L][SABER_L][SABER_N],
                         const int16_t s[SABER_L][SABER_N],
                         int16_t res[SABER_L][SABER_N], int16_t transpose);
void MatrixVectorMulKP_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                           uint16_t b[SABER_L][SABER_N]);
void MatrixVectorMulEnc_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext);
#endif
