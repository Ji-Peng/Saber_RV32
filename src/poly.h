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
void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES]);

void poly_basemul(int32_t r[SABER_N], const int32_t a[SABER_N],
                  const int32_t b[SABER_N]);

void poly_add(int16_t res[SABER_N], int32_t in[SABER_N]);

void InnerProd_ntt(const int16_t b[SABER_L][SABER_N],
                   const int16_t s[SABER_L][SABER_N], int16_t res[SABER_N]);
#endif
