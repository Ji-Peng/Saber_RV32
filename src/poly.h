#ifndef POLY_H
#define POLY_H

#include <stdint.h>

#include "SABER_params.h"

void MatrixVectorMul(const uint16_t a[SABER_L][SABER_L][SABER_N],
                     const uint16_t s[SABER_L][SABER_N],
                     uint16_t res[SABER_L][SABER_N], int16_t transpose);
void InnerProd(const uint16_t b[SABER_L][SABER_N],
               const uint16_t s[SABER_L][SABER_N], uint16_t res[SABER_N]);
void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES]);
void GenSecretInTime(uint16_t s[SABER_N],
                     const uint8_t seed[SABER_NOISE_SEEDBYTES], int32_t index);
void GenSecret_ntt(int32_t s[SABER_L][SABER_N],
                   const uint8_t seed[SABER_NOISE_SEEDBYTES]);
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
#ifdef SLOWGENA_FASTMUL
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             int32_t x, int32_t y);

#else
void GenPoly(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
             uint32_t init);
#endif

#ifdef FASTGENA_SLOWMUL
void MatrixVectorMulEnc_ntt(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext);
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                            const uint16_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext,
                            const uint8_t m[SABER_KEYBYTES]);
#else
void MatrixVectorMulEnc_ntt(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext);
void InnerProdInTimeEnc_ntt(const uint8_t *bytes,
                            const int32_t s[SABER_L][SABER_N],
                            uint8_t *ciphertext,
                            const uint8_t m[SABER_KEYBYTES]);
#endif

#endif
