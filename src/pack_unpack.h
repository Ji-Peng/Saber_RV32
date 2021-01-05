#ifndef PACK_UNPACK_H
#define PACK_UNPACK_H

#include <stdint.h>
#include <stdio.h>

#include "SABER_params.h"

void SABER_pack_3bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack3bit(uint8_t *bytes, uint16_t *data);

void SABER_pack_4bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack4bit(const unsigned char *bytes, uint16_t *mask_ar);

void SABER_pack_6bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack6bit(const unsigned char *bytes, uint16_t *data);

void POLVECp2BS(uint8_t *pk, uint16_t skpv[SABER_K][SABER_N]);

void BS2POLVECp(const unsigned char *pk, uint16_t data[SABER_K][SABER_N]);

void POLVECq2BS(uint8_t *sk, uint16_t skpv[SABER_K][SABER_N]);

void BS2POLVECq(const unsigned char *sk, uint16_t skpv[SABER_K][SABER_N]);

void BS2POLp(uint16_t pol_index, const unsigned char *bytes, uint16_t pol[]);

void BS2POLq(const unsigned char *bytes, uint16_t data[SABER_N]);

void POLp2BS(uint8_t *bytes, uint16_t data[SABER_N], uint16_t pol_vec_index);

#endif
