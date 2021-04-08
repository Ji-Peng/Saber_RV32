#include <stdint.h>

#include "SABER_params.h"

int32_t FqMul(int32_t a, int32_t b);
void NTTA(const uint16_t in[256], int32_t out[256]);
void NTTS(const uint8_t in[256], int32_t out[256]);
void InvNTT(int32_t in[256], int32_t out[256]);
#ifdef SIX_LAYER_NTT
void BaseMul(int32_t a[4], const int32_t b[4], int32_t zeta);
#elif defined(SEVEN_LAYER_NTT)
void BaseMul(int32_t a[2], const int32_t b[2], int32_t zeta);
#elif defined(COMPLETE_NTT)
void BaseMul(int32_t a[SABER_N], const int32_t b[SABER_N]);
#endif