#ifndef POLY_MUL_H
#define POLY_MUL_H

#include <stdint.h>

#include "SABER_params.h"

void PolyBaseMul(int32_t a[SABER_N], const int32_t b[SABER_N]);

void PolyAdd(uint16_t res[SABER_N], int32_t in[SABER_N]);

void PolyMulAcc(uint16_t a[2 * SABER_N], const uint16_t b[SABER_N],
                uint16_t res[SABER_N]);
void PolyMulAccFast(uint16_t a[SABER_N], const int32_t b[SABER_N],
                    uint16_t res[SABER_N]);
void MatrixVectorMulKP(const uint8_t *seed_a, const uint8_t *seed_s,
                       uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                       uint16_t b[SABER_L][SABER_N]);
#if defined(FASTGENA_SLOWMUL)
void MatrixVectorMulEnc(const uint8_t *seed, uint16_t s[SABER_L][SABER_N],
                        uint8_t *ciphertext);
void InnerProdInTimeEnc(const uint8_t *bytes,
                        const uint16_t s[SABER_L][SABER_N], uint8_t *ciphertext,
                        const uint8_t m[SABER_KEYBYTES]);
void InnerProdInTimeDec(const uint8_t *bytes,
                        const uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                        uint16_t res[SABER_N]);
#elif defined(FASTGENA_FASTMUL) || defined(SLOWGENA_FASTMUL)
void MatrixVectorMulEnc(const uint8_t *seed, int32_t s[SABER_L][SABER_N],
                        uint8_t *ciphertext);
void InnerProdInTimeEnc(const uint8_t *bytes, const int32_t s[SABER_L][SABER_N],
                        uint8_t *ciphertext, const uint8_t m[SABER_KEYBYTES]);
#endif
#endif