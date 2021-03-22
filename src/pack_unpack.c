#include "pack_unpack.h"

#include <string.h>

#include "api.h"

void PolT2BS(uint8_t bytes[SABER_SCALEBYTES_KEM], const uint16_t data[SABER_N])
{
    int32_t j, offsetByte, offsetData;
#if SABER_ET == 3
    for (j = 0; j < SABER_N / 8; j++) {
        offsetByte = 3 * j;
        offsetData = 8 * j;
        bytes[offsetByte + 0] = (data[offsetData + 0] & 0x7) |
                                 ((data[offsetData + 1] & 0x7) << 3) |
                                 ((data[offsetData + 2] & 0x3) << 6);
        bytes[offsetByte + 1] = ((data[offsetData + 2] >> 2) & 0x01) |
                                 ((data[offsetData + 3] & 0x7) << 1) |
                                 ((data[offsetData + 4] & 0x7) << 4) |
                                 (((data[offsetData + 5]) & 0x01) << 7);
        bytes[offsetByte + 2] = ((data[offsetData + 5] >> 1) & 0x03) |
                                 ((data[offsetData + 6] & 0x7) << 2) |
                                 ((data[offsetData + 7] & 0x7) << 5);
    }
#elif SABER_ET == 4
    for (j = 0; j < SABER_N / 2; j++) {
        offsetByte = j;
        offsetData = 2 * j;
        bytes[offsetByte] =
            (data[offsetData] & 0x0f) | ((data[offsetData + 1] & 0x0f) << 4);
    }
#elif SABER_ET == 6
    for (j = 0; j < SABER_N / 4; j++) {
        offsetByte = 3 * j;
        offsetData = 4 * j;
        bytes[offsetByte + 0] = (data[offsetData + 0] & 0x3f) |
                                 ((data[offsetData + 1] & 0x03) << 6);
        bytes[offsetByte + 1] = ((data[offsetData + 1] >> 2) & 0x0f) |
                                 ((data[offsetData + 2] & 0x0f) << 4);
        bytes[offsetByte + 2] = ((data[offsetData + 2] >> 4) & 0x03) |
                                 ((data[offsetData + 3] & 0x3f) << 2);
    }
#else
#    error "Unsupported SABER parameter."
#endif
}

void BS2PolT(const uint8_t bytes[SABER_SCALEBYTES_KEM], uint16_t data[SABER_N])
{
    int32_t j, offsetByte, offsetData;
#if SABER_ET == 3
    for (j = 0; j < SABER_N / 8; j++) {
        offsetByte = 3 * j;
        offsetData = 8 * j;
        data[offsetData + 0] = (bytes[offsetByte + 0]) & 0x07;
        data[offsetData + 1] = ((bytes[offsetByte + 0]) >> 3) & 0x07;
        data[offsetData + 2] = (((bytes[offsetByte + 0]) >> 6) & 0x03) |
                                (((bytes[offsetByte + 1]) & 0x01) << 2);
        data[offsetData + 3] = ((bytes[offsetByte + 1]) >> 1) & 0x07;
        data[offsetData + 4] = ((bytes[offsetByte + 1]) >> 4) & 0x07;
        data[offsetData + 5] = (((bytes[offsetByte + 1]) >> 7) & 0x01) |
                                (((bytes[offsetByte + 2]) & 0x03) << 1);
        data[offsetData + 6] = ((bytes[offsetByte + 2] >> 2) & 0x07);
        data[offsetData + 7] = ((bytes[offsetByte + 2] >> 5) & 0x07);
    }
#elif SABER_ET == 4
    for (j = 0; j < SABER_N / 2; j++) {
        offsetByte = j;
        offsetData = 2 * j;
        data[offsetData] = bytes[offsetByte] & 0x0f;
        data[offsetData + 1] = (bytes[offsetByte] >> 4) & 0x0f;
    }
#elif SABER_ET == 6
    for (j = 0; j < SABER_N / 4; j++) {
        offsetByte = 3 * j;
        offsetData = 4 * j;
        data[offsetData + 0] = bytes[offsetByte + 0] & 0x3f;
        data[offsetData + 1] = ((bytes[offsetByte + 0] >> 6) & 0x03) |
                                ((bytes[offsetByte + 1] & 0x0f) << 2);
        data[offsetData + 2] = ((bytes[offsetByte + 1] & 0xff) >> 4) |
                                ((bytes[offsetByte + 2] & 0x03) << 4);
        data[offsetData + 3] = ((bytes[offsetByte + 2] & 0xff) >> 2);
    }
#else
#    error "Unsupported SABER parameter."
#endif
}

/**
 * @description: 12*13bytes=156bytes <==> 12*8=96coefficients or 6*13=78bytes
 * <==> 6*8=48coefficients
 */
void BS2Polq(const uint8_t *bytes, uint16_t *data)
{
    int32_t j, offsetByte, offsetData;
    for (j = 0; j < SABER_N / 8; j++) {
        offsetByte = 13 * j;
        offsetData = 8 * j;
        data[offsetData + 0] = (bytes[offsetByte + 0] & (0xff)) |
                                ((bytes[offsetByte + 1] & 0x1f) << 8);
        data[offsetData + 1] = (bytes[offsetByte + 1] >> 5 & (0x07)) |
                                ((bytes[offsetByte + 2] & 0xff) << 3) |
                                ((bytes[offsetByte + 3] & 0x03) << 11);
        data[offsetData + 2] = (bytes[offsetByte + 3] >> 2 & (0x3f)) |
                                ((bytes[offsetByte + 4] & 0x7f) << 6);
        data[offsetData + 3] = (bytes[offsetByte + 4] >> 7 & (0x01)) |
                                ((bytes[offsetByte + 5] & 0xff) << 1) |
                                ((bytes[offsetByte + 6] & 0x0f) << 9);
        data[offsetData + 4] = (bytes[offsetByte + 6] >> 4 & (0x0f)) |
                                ((bytes[offsetByte + 7] & 0xff) << 4) |
                                ((bytes[offsetByte + 8] & 0x01) << 12);
        data[offsetData + 5] = (bytes[offsetByte + 8] >> 1 & (0x7f)) |
                                ((bytes[offsetByte + 9] & 0x3f) << 7);
        data[offsetData + 6] = (bytes[offsetByte + 9] >> 6 & (0x03)) |
                                ((bytes[offsetByte + 10] & 0xff) << 2) |
                                ((bytes[offsetByte + 11] & 0x07) << 10);
        data[offsetData + 7] = (bytes[offsetByte + 11] >> 3 & (0x1f)) |
                                ((bytes[offsetByte + 12] & 0xff) << 5);
    }
}

void Polp2BS(uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             const uint16_t data[SABER_N])
{
    int32_t j, offsetByte, offsetData;
    for (j = 0; j < SABER_N / 4; j++) {
        offsetByte = 5 * j;
        offsetData = 4 * j;
        bytes[offsetByte + 0] = (data[offsetData + 0] & (0xff));
        bytes[offsetByte + 1] = ((data[offsetData + 0] >> 8) & 0x03) |
                                 ((data[offsetData + 1] & 0x3f) << 2);
        bytes[offsetByte + 2] = ((data[offsetData + 1] >> 6) & 0x0f) |
                                 ((data[offsetData + 2] & 0x0f) << 4);
        bytes[offsetByte + 3] = ((data[offsetData + 2] >> 4) & 0x3f) |
                                 ((data[offsetData + 3] & 0x03) << 6);
        bytes[offsetByte + 4] = ((data[offsetData + 3] >> 2) & 0xff);
    }
}

void BS2Polp(const uint8_t bytes[SABER_POLYCOMPRESSEDBYTES],
             uint16_t data[SABER_N])
{
    int32_t j, offsetByte, offsetData;
    for (j = 0; j < SABER_N / 4; j++) {
        offsetByte = 5 * j;
        offsetData = 4 * j;
        data[offsetData + 0] = (bytes[offsetByte + 0] & (0xff)) |
                                ((bytes[offsetByte + 1] & 0x03) << 8);
        data[offsetData + 1] = ((bytes[offsetByte + 1] >> 2) & (0x3f)) |
                                ((bytes[offsetByte + 2] & 0x0f) << 6);
        data[offsetData + 2] = ((bytes[offsetByte + 2] >> 4) & (0x0f)) |
                                ((bytes[offsetByte + 3] & 0x3f) << 4);
        data[offsetData + 3] = ((bytes[offsetByte + 3] >> 6) & (0x03)) |
                                ((bytes[offsetByte + 4] & 0xff) << 2);
    }
}

void PackSk(uint8_t bytes[SABER_SKPOLYBYTES], const uint16_t data[SABER_N])
{
    int32_t j, offsetData;
    for (j = 0; j < SABER_N / 2; j++) {
        offsetData = 2 * j;
        bytes[j] = (data[offsetData + 1] << 4) | (data[offsetData] & 0xf);
    }
}

void UnpackSk(const uint8_t bytes[SABER_INDCPA_SECRETKEYBYTES],
              uint16_t data[SABER_L][SABER_N])
{
    int32_t i, j, offsetByte, offsetData;
    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_N / 2; j++) {
            offsetByte = i * SABER_SKPOLYBYTES + j;
            offsetData = 2 * j;
            data[i][offsetData] =
                (int16_t)((int16_t)bytes[offsetByte] << 12) >> 12;
            data[i][offsetData + 1] =
                (int16_t)((bytes[offsetByte] >> 4) << 12) >> 12;
        }
    }
}

void PolVecp2BS(uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                const uint16_t data[SABER_L][SABER_N])
{
    int32_t i;
    for (i = 0; i < SABER_L; i++) {
        Polp2BS(bytes + i * (SABER_EP * SABER_N / 8), data[i]);
    }
}

void BS2PolVecp(const uint8_t bytes[SABER_POLYVECCOMPRESSEDBYTES],
                uint16_t data[SABER_L][SABER_N])
{
    int32_t i;
    for (i = 0; i < SABER_L; i++) {
        BS2Polp(bytes + i * (SABER_EP * SABER_N / 8), data[i]);
    }
}

void BS2PolMsg(const uint8_t bytes[SABER_KEYBYTES], uint16_t data[SABER_N])
{
    int32_t i, j;
    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            data[j * 8 + i] = ((bytes[j] >> i) & 0x01);
        }
    }
}

void PolMsg2BS(uint8_t bytes[SABER_KEYBYTES], const uint16_t data[SABER_N])
{
    int32_t i, j;
    memset(bytes, 0, SABER_KEYBYTES);

    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            bytes[j] = bytes[j] | ((data[j * 8 + i] & 0x01) << i);
        }
    }
}