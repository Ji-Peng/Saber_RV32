#include <stdint.h>

int32_t FqMul(int32_t a, int32_t b);
void NTT(const uint16_t in[256], int32_t out[256]);
void InvNTT(int32_t in[256], int32_t out[256]);
void BaseMul(int32_t a[4], const int32_t b[4], int32_t zeta);
void InvNTTAsm(int32_t in[256], int32_t out[256]);
int32_t FqMulAsm(int32_t a, int32_t b);
int32_t BarrettReduceAsm(int32_t a);