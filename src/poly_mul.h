#ifndef POLY_MUL_H
#define POLY_MUL_H

#include <stdint.h>

#include "SABER_params.h"

void poly_mul_acc(const uint16_t a[SABER_N], const uint16_t b[SABER_N],
                  uint16_t res[SABER_N]);

#endif