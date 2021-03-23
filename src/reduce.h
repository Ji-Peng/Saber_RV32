#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>

#define M 5243393          // M = 512 * 10241 + 1 < 2^23
#define Mprime -934542849  // M * Mprime = -1 mod R (R=2^32)
#define MINV 934542849     // M * MINV = 1 mod R
#define RmodM 628429       // R mod M
#define NINV 1082784       // R^2 * (1/64) mod M

int32_t MontReduce(int64_t a);

int32_t BarrettReduce(int32_t a);

#endif
