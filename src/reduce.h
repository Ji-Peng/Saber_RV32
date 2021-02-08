#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>

// q' for ntt
#define M 25166081
// M * Mprime = -1 mod R
#define Mprime 41877759
// M * MINV = 1 mod R
#define MINV 4253089537
// R is constant in montgomery reduce
#define RmodM -8432555
// 1/256 in Mont Field
#define NINV 7689784

int32_t montgomery_reduce(int64_t a);

int32_t barrett_reduce(int32_t a);

#endif
