#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>

// #define M 25166081       // M = 196610 * 128 + 1
// #define Mprime 41877759  // M * Mprime = -1 mod R (R=2^32)
// #define MINV 4253089537  // M * MINV = 1 mod R
// #define RmodM -8432555   // R mod M
// #define NINV 7689784     // R^2 * (1/64) mod M

#define M 4205569           // M = 8214 * 512 + 1 < 2^23
#define Mprime -1196413953  // M * Mprime = -1 mod R (R=2^32)
#define MINV 1196413953     // M * MINV = 1 mod R
#define RmodM 1081347       // R mod M
#define NINV 906456         // R^2 * (1/64) mod M

int32_t MontReduce(int64_t a);

int32_t BarrettReduce(int32_t a);

#endif
