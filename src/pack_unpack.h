#ifndef PACK_UNPACK_H
#define PACK_UNPACK_H

#include <stdio.h>
#include <stdint.h>
#include "SABER_params.h"


void SABER_pack_3bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack3bit(uint8_t *bytes, uint16_t *data);

void SABER_pack_4bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack4bit(const unsigned char *bytes, uint16_t *ar);

void SABER_pack_6bit(uint8_t *bytes, uint16_t *data);

void SABER_un_pack6bit(const unsigned char *bytes, uint16_t *data);


void POLVECp2BS(uint8_t *pk,  uint16_t skpv[SABER_K][SABER_N]);

void BS2POLVECp(const unsigned char *pk, uint16_t data[SABER_K][SABER_N]);

//void POLVECq2BS(uint8_t *sk,  uint16_t skpv[SABER_K][SABER_N]);

//void BS2POLVECq(const unsigned char *sk,  uint16_t skpv[SABER_K][SABER_N]);


void BS2POL(const unsigned char *bytes, uint16_t data[SABER_N]);

void POLVEC2BS(uint8_t *bytes, uint16_t data[SABER_K][SABER_N], uint16_t modulus);

void BS2POLVEC(const unsigned char *bytes, uint16_t data[SABER_K][SABER_N], uint16_t modulus);

//compressed secrets
void POLSEC2BS(uint8_t *bytes, int8_t data[SABER_K][SABER_N]);

void BS2POLSEC(const unsigned char *bytes, int8_t data[SABER_K][SABER_N]);

void POLp2BS(uint8_t *bytes, uint16_t data[SABER_N], uint16_t pol_vec_index);

//jit
void BS2POLp(uint16_t pol_index, const unsigned char *bytes, uint16_t pol[]);

//in-place cmp
unsigned char POLp2BS_cmp(uint8_t *bytes, uint16_t data[SABER_N], uint16_t pol_vec_index);

unsigned char SABER_pack_3bit_cmp(uint8_t *bytes, uint16_t *data);

unsigned char SABER_pack_4bit_cmp(uint8_t *bytes, uint16_t *data);

unsigned char SABER_pack_6bit_cmp(uint8_t *bytes, uint16_t *data);

#endif
