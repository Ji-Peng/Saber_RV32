#include <stdint.h>

#include "SABER_params.h"

int32_t FqMul(int32_t a, int32_t b);
void NTT(const uint16_t in[256], int32_t out[256]);
void InvNTT(int32_t in[256], int32_t out[256]);
#ifdef SIX_LAYER_NTT
void BaseMul(int32_t a[4], const int32_t b[4], int32_t zeta);
#elif defined(SEVEN_LAYER_NTT)
void BaseMul(int32_t a[SABER_N], const int32_t b[SABER_N]);
#elif defined(COMPLETE_NTT)
void BaseMul(int32_t a[SABER_N], const int32_t b[SABER_N]);
#elif defined(FIVE_LAYER_NTT)
void BaseMul(int32_t a[8], const int32_t b[8], int32_t zeta);
#endif