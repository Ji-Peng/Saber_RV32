#include <stdint.h>

int32_t fqmul(int32_t a, int32_t b);
void ntt(const int16_t in[256], int32_t out[256]);
void ntt_merged(const int16_t in[256], int32_t out[256]);
void ntt_merged_old(const int16_t in[256], int32_t out[256]);
void invntt(int32_t in[256], int32_t out[256]);