#ifndef POLY_H
#define POLY_H

#include <stdint.h>

#include "SABER_params.h"

void GenSecret(uint8_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES]);
void GenSInTime(uint8_t s[SABER_N], const uint8_t seed[SABER_NOISE_SEEDBYTES],
                int32_t index);
void GenSecretNTT(int32_t s[SABER_L][SABER_N],
                  const uint8_t seed[SABER_NOISE_SEEDBYTES]);
void CenteredReduce(uint16_t poly[SABER_N]);
#if defined(FASTGENA_SLOWMUL) || defined(FASTGENA_FASTMUL)
void GenAInTime(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
                uint32_t init);

#elif defined(SLOWGENA_FASTMUL)
void GenAInTime(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
                int32_t x, int32_t y);
#endif

#endif
