#include <stdint.h>

#include "SABER_params.h"

int32_t FqMul(int32_t a, int32_t b);
void NTT(const uint16_t in[256], int32_t out[256]);
void InvNTT(int32_t in[256], int32_t out[256]);
void PolyBaseMul(int32_t a[SABER_N], const int32_t b[SABER_N]);