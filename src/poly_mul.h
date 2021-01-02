#ifndef POLY_MUL_H
#define POLY_MUL_H

#include <stdint.h>

#include "SABER_params.h"

void poly_mul_acc(const uint16_t a[SABER_N], const uint16_t b[SABER_N],
                  uint16_t res[SABER_N]);
void MatrixVectorMul_encryption(const unsigned char* seed,
                                uint16_t sp[SABER_L][SABER_N],
                                unsigned char* ciphertext);
void MatrixVectorMul_keypair(const unsigned char* seed,
                             uint16_t s[SABER_L][SABER_N],
                             uint16_t b[SABER_L][SABER_N]);
void VectorMul(const unsigned char* bytes, uint16_t sp[SABER_L][SABER_N],
               uint16_t res[SABER_N]);
#endif