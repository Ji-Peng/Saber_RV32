#include <stdint.h>

int32_t fqmul(int32_t a, int32_t b);
void ntt(const int16_t in[256], int32_t out[256]);
void invntt(int32_t in[256], int32_t out[256]);
void basemul(int32_t r[4], const int32_t a[4], const int32_t b[4],
             int32_t zeta);