#include "poly.h"

#include <stdio.h>

#include "api.h"
#include "cbd.h"
#include "fips202.h"
#include "pack_unpack.h"
#include "poly_mul.h"

void MatrixVectorMul(const uint16_t A[SABER_L][SABER_L][SABER_N],
                     const uint16_t s[SABER_L][SABER_N],
                     uint16_t res[SABER_L][SABER_N], int16_t transpose)
{
    int i, j;
    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_L; j++) {
            if (transpose == 1) {
                poly_mul_acc(A[j][i], s[j], res[i]);
            } else {
                poly_mul_acc(A[i][j], s[j], res[i]);
            }
        }
    }
}

void InnerProd(const uint16_t b[SABER_L][SABER_N],
               const uint16_t s[SABER_L][SABER_N], uint16_t res[SABER_N])
{
    int j;
    for (j = 0; j < SABER_L; j++) {
        poly_mul_acc(b[j], s[j], res);
    }
}

void GenMatrix(uint16_t A[SABER_L][SABER_L][SABER_N],
               const uint8_t seed[SABER_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYVECBYTES];
    int i;

    shake128(buf, sizeof(buf), seed, SABER_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        BS2POLVECq(buf + i * SABER_POLYVECBYTES, A[i]);
    }
}

void GenSecret(uint16_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYCOINBYTES];
    size_t i;

    shake128(buf, sizeof(buf), seed, SABER_NOISE_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        cbd(s[i], buf + i * SABER_POLYCOINBYTES);
    }
}

void byte_bank2pol_part(unsigned char *bytes, uint16_t pol_part[],
                        uint16_t pol_part_start_index, uint16_t num_8coeff)
{
    uint32_t j;
    uint32_t offset_data = 0, offset_byte = 0;

    offset_byte = 0;

    for (j = 0; j < num_8coeff; j++) {
        offset_byte = 13 * j;
        offset_data = pol_part_start_index + 8 * j;
        pol_part[offset_data + 0] = (bytes[offset_byte + 0] & (0xff)) |
                                    ((bytes[offset_byte + 1] & 0x1f) << 8);
        pol_part[offset_data + 1] = (bytes[offset_byte + 1] >> 5 & (0x07)) |
                                    ((bytes[offset_byte + 2] & 0xff) << 3) |
                                    ((bytes[offset_byte + 3] & 0x03) << 11);
        pol_part[offset_data + 2] = (bytes[offset_byte + 3] >> 2 & (0x3f)) |
                                    ((bytes[offset_byte + 4] & 0x7f) << 6);
        pol_part[offset_data + 3] = (bytes[offset_byte + 4] >> 7 & (0x01)) |
                                    ((bytes[offset_byte + 5] & 0xff) << 1) |
                                    ((bytes[offset_byte + 6] & 0x0f) << 9);
        pol_part[offset_data + 4] = (bytes[offset_byte + 6] >> 4 & (0x0f)) |
                                    ((bytes[offset_byte + 7] & 0xff) << 4) |
                                    ((bytes[offset_byte + 8] & 0x01) << 12);
        pol_part[offset_data + 5] = (bytes[offset_byte + 8] >> 1 & (0x7f)) |
                                    ((bytes[offset_byte + 9] & 0x3f) << 7);
        pol_part[offset_data + 6] = (bytes[offset_byte + 9] >> 6 & (0x03)) |
                                    ((bytes[offset_byte + 10] & 0xff) << 2) |
                                    ((bytes[offset_byte + 11] & 0x07) << 10);
        pol_part[offset_data + 7] = (bytes[offset_byte + 11] >> 3 & (0x1f)) |
                                    ((bytes[offset_byte + 12] & 0xff) << 5);
    }
}

void GenMatrix_poly(uint16_t temp[], const unsigned char *seed,
                    uint16_t poly_number)
{
    // there can be at most 112 bytes left over from previous shake call
    static unsigned char shake_op_buf[SHAKE128_RATE + 112];

    static int i, j;

    static uint64_t s[25];

    static uint16_t pol_part_start_index, num_8coeff, num_8coeff_final,
        left_over_bytes, total_bytes;
    static uint16_t poly_complete;

    // Init state when poly_number=0;

    if (poly_number == 0) {
        for (i = 0; i < 25; ++i)
            s[i] = 0;

        keccak_absorb(s, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);

        pol_part_start_index = 0;
        num_8coeff = 0;
        left_over_bytes = 0;
        total_bytes = 0;
    }

    poly_complete = 0;

    while (poly_complete != 1) {
        keccak_squeezeblocks(shake_op_buf + left_over_bytes, 1, s,
                             SHAKE128_RATE);

        total_bytes = left_over_bytes + SHAKE128_RATE;

        num_8coeff = total_bytes / 13;

        if ((num_8coeff * 8 + pol_part_start_index) > 255)
            num_8coeff_final = 32 - pol_part_start_index / 8;
        else
            num_8coeff_final = num_8coeff;

        byte_bank2pol_part(shake_op_buf, temp, pol_part_start_index,
                           num_8coeff_final);

        left_over_bytes = total_bytes - num_8coeff_final * 13;
        // bring the leftover in the begining of the buffer.
        for (j = 0; j < left_over_bytes; j++)
            shake_op_buf[j] = shake_op_buf[num_8coeff_final * 13 + j];
        // this will be >256 when the polynomial is complete.
        pol_part_start_index = pol_part_start_index + num_8coeff_final * 8;

        if (pol_part_start_index > 255) {
            pol_part_start_index = 0;
            poly_complete++;
        }
    }
}