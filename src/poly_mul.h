#ifndef POLY_MUL_H
#define POLY_MUL_H

#include <stdint.h>

#include "SABER_params.h"

void poly_basemul(int32_t a[SABER_N], const int32_t b[SABER_N]);

void poly_add(uint16_t res[SABER_N], int32_t in[SABER_N]);

void poly_mul_acc_ntt(uint16_t a[2 * SABER_N], const uint16_t b[SABER_N],
                      uint16_t res[SABER_N]);
void poly_mul_acc_ntt_fast(uint16_t a[SABER_N], const int32_t b[SABER_N],
                           uint16_t res[SABER_N]);
void InnerProd_ntt(const int16_t b[SABER_L][SABER_N],
                   const int16_t s[SABER_L][SABER_N], int16_t res[SABER_N]);
void InnerProdInTime_ntt(const uint8_t *bytes,
                         const uint16_t s[SABER_L][SABER_N],
                         uint16_t res[SABER_N]);
void MatrixVectorMul_ntt(const int16_t A[SABER_L][SABER_L][SABER_N],
                         const int16_t s[SABER_L][SABER_N],
                         int16_t res[SABER_L][SABER_N], int16_t transpose);
void MatrixVectorMulKP_ntt(const uint8_t *seed_a, const uint8_t *seed_s,
                           uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                           uint16_t b[SABER_L][SABER_N]);
#if defined(FASTGENA_SLOWMUL)
void MatrixVectorMulEnc_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext);
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                            const uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext,
                            const uint8_t m[SABER_KEYBYTES]);
#elif defined(FASTGENA_FASTMUL) || defined(SLOWGENA_FASTMUL)
void MatrixVectorMulEnc_ntt(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext);
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                            const int32_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext,
                            const uint8_t m[SABER_KEYBYTES]);
#endif
#endif