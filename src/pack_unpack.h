#ifndef PACK_UNPACK_H
#define PACK_UNPACK_H

#include <stdint.h>
#include <stdio.h>

#include "SABER_params.h"

void PolT2BS(uint8_t bytes[SABER_SCALEBYTES_KEM], const uint16_t data[SABER_N]);
int32_t PolT2BSCmp(const uint8_t bytes[SABER_SCALEBYTES_KEM],
                   const uint16_t data[SABER_N]);
void BS2PolT(const uint8_t bytes[SABER_SCALEBYTES_KEM], uint16_t data[SABER_N]);
void PolVecp2BS(uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                const uint16_t data[SABER_L][SABER_N]);
void BS2PolVecp(const uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                uint16_t data[SABER_L][SABER_N]);

void BS2PolMsg(const uint8_t bytes[SABER_KEYBYTES], uint16_t data[SABER_N]);
void PolMsg2BS(uint8_t bytes[SABER_KEYBYTES], const uint16_t data[SABER_N]);

void BS2Polq(const uint8_t *bytes, uint16_t *data);

void BS2Polp(const uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             uint16_t data[SABER_N]);
void Polp2BS(uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             const uint16_t data[SABER_N]);
int32_t Polp2BSCmp(const uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
                   const uint16_t data[SABER_N]);
void PackSk(uint8_t bytes[SABER_SKPOLYBYTES], const uint8_t data[SABER_N]);
void UnpackSk(const uint8_t *bytes, uint8_t data[SABER_N]);
#endif
