#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>

#define M 10487809          // M = 512 * 20484 + 1 < 2^24
#define Mprime -4288673793  // M * Mprime = -1 mod R (R=2^32)
#define MINV 4288673793     // M * MINV = 1 mod R
#define RmodM 5453415       // R mod M
#define NINV 2466627        // R^2 * (1/64) mod M

int32_t MontReduce(int64_t a);

int32_t BarrettReduce(int32_t a);

#endif
