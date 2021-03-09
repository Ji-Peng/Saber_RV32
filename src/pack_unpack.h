#ifndef PACK_UNPACK_H
#define PACK_UNPACK_H

#include <stdint.h>
#include <stdio.h>

#include "SABER_params.h"

void POLT2BS(uint8_t bytes[SABER_SCALEBYTES_KEM], const uint16_t data[SABER_N]);
void BS2POLT(const uint8_t bytes[SABER_SCALEBYTES_KEM], uint16_t data[SABER_N]);

// void POLVECq2BS(uint8_t bytes[SABER_POLYVECBYTES],
//                 const uint16_t data[SABER_L][SABER_N]);
void POLVECp2BS(uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                const uint16_t data[SABER_L][SABER_N]);

void BS2POLVECq(const uint8_t bytes[SABER_POLYVECBYTES],
                uint16_t data[SABER_L][SABER_N]);
void BS2POLVECp(const uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                uint16_t data[SABER_L][SABER_N]);

void BS2POLmsg(const uint8_t bytes[SABER_KEYBYTES], uint16_t data[SABER_N]);
void POLmsg2BS(uint8_t bytes[SABER_KEYBYTES], const uint16_t data[SABER_N]);

void BS2POLq(const uint8_t bytes[SABER_POLYBYTES], uint16_t data[SABER_N]);
void BS2POLp(const uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             uint16_t data[SABER_N]);
void POLp2BS(uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             const uint16_t data[SABER_N]);

void pack_sk(uint8_t bytes[SABER_INDCPA_SECRETKEYBYTES],
             const uint16_t data[SABER_L][SABER_N]);
void unpack_sk(const uint8_t bytes[SABER_INDCPA_SECRETKEYBYTES],
               uint16_t data[SABER_L][SABER_N]);
#endif
