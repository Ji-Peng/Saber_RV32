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
void GenMatrix_poly(uint16_t temp[], const unsigned char *seed,
                    uint16_t poly_number);
#endif