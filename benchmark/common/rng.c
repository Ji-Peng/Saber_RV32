//
//  rng.c
//
//  Created by Bassham, Lawrence E (Fed) on 8/29/17.
//  Copyright © 2017 Bassham, Lawrence E (Fed). All rights reserved.
//
#include "rng.h"

#include <string.h>

#include "aes.h"

AES256_CTR_DRBG_struct DRBG_ctx;

void AES256_ECB(unsigned char *key, unsigned char *ctr, unsigned char *buffer);

/**
 * @description: AES256 ECB mode
 * @param {unsignedchar} *key: AES key (32bytes)
 * @param {unsignedchar} *in: input 128-bit == 16bytes plaintext value
 * @param {unsignedchar} *out: output 128-bit == 16bytes ciphertext value
 * A block in AES is 16bytes == 128bits
 */
void AES256_ECB(unsigned char *key, unsigned char *in, unsigned char *out)
{
    aes256ctx ctx;

    // initialise the context
    aes256_ecb_keyexp(&ctx, key);
    // invoke aes256_ecb encryption
    aes256_ecb(out, in, 1, &ctx);
    // clean up the context
    aes256_ctx_release(&ctx);
}

void RandomBytesInit(unsigned char *entropy_input,
                      unsigned char *personalization_string)
{
    unsigned char seed_material[48];

    memcpy(seed_material, entropy_input, 48);
    if (personalization_string)
        for (int i = 0; i < 48; i++)
            seed_material[i] ^= personalization_string[i];
    memset(DRBG_ctx.Key, 0x00, 32);
    memset(DRBG_ctx.V, 0x00, 16);
    AES256_CTR_DRBG_Update(seed_material, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter = 1;
}

int RandomBytes(unsigned char *x, unsigned long long xlen)
{
    unsigned char block[16];
    int i = 0;

    while (xlen > 0) {
        // increment V
        for (int j = 15; j >= 0; j--) {
            if (DRBG_ctx.V[j] == 0xff)
                DRBG_ctx.V[j] = 0x00;
            else {
                DRBG_ctx.V[j]++;
                break;
            }
        }
        AES256_ECB(DRBG_ctx.Key, DRBG_ctx.V, block);
        if (xlen > 15) {
            memcpy(x + i, block, 16);
            i += 16;
            xlen -= 16;
        } else {
            memcpy(x + i, block, xlen);
            xlen = 0;
        }
    }
    AES256_CTR_DRBG_Update(NULL, DRBG_ctx.Key, DRBG_ctx.V);
    DRBG_ctx.reseed_counter++;

    return RNG_SUCCESS;
}

void AES256_CTR_DRBG_Update(unsigned char *provided_data, unsigned char *Key,
                            unsigned char *V)
{
    unsigned char temp[48];

    for (int i = 0; i < 3; i++) {
        // increment V
        for (int j = 15; j >= 0; j--) {
            if (V[j] == 0xff)
                V[j] = 0x00;
            else {
                V[j]++;
                break;
            }
        }

        AES256_ECB(Key, V, temp + 16 * i);
    }
    if (provided_data != NULL)
        for (int i = 0; i < 48; i++)
            temp[i] ^= provided_data[i];
    memcpy(Key, temp, 32);
    memcpy(V, temp + 32, 16);
}