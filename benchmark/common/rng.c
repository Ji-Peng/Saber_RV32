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

/*
 ctx            - stores the current state of an instance of the seed expander
 seed           - a 32 byte random value
 diversifier    - an 8 byte diversifier
 maxlen         - maximum number of bytes (less than 2**32) generated under this
 seed and diversifier
 */
// int seedexpander_init(AES_XOF_struct *ctx, unsigned char *seed,
//                       unsigned char *diversifier, unsigned long maxlen)
// {
//     if (maxlen >= 0x100000000)
//         return RNG_BAD_MAXLEN;

//     ctx->length_remaining = maxlen;

//     memcpy(ctx->key, seed, 32);

//     memcpy(ctx->ctr, diversifier, 8);
//     ctx->ctr[11] = maxlen % 256;
//     maxlen >>= 8;
//     ctx->ctr[10] = maxlen % 256;
//     maxlen >>= 8;
//     ctx->ctr[9] = maxlen % 256;
//     maxlen >>= 8;
//     ctx->ctr[8] = maxlen % 256;
//     memset(ctx->ctr + 12, 0x00, 4);

//     ctx->buffer_pos = 16;
//     memset(ctx->buffer, 0x00, 16);

//     return RNG_SUCCESS;
// }

/*
    ctx  - stores the current state of an instance of the seed expander
    x    - returns the XOF data
    xlen - number of bytes to return
 */
// int seedexpander(AES_XOF_struct *ctx, unsigned char *x, unsigned long xlen)
// {
//     unsigned long offset;

//     if (x == NULL)
//         return RNG_BAD_OUTBUF;
//     if (xlen >= ctx->length_remaining)
//         return RNG_BAD_REQ_LEN;

//     ctx->length_remaining -= xlen;

//     offset = 0;
//     while (xlen > 0) {
//         if (xlen <= (16 - ctx->buffer_pos)) {  // buffer has what we need
//             memcpy(x + offset, ctx->buffer + ctx->buffer_pos, xlen);
//             ctx->buffer_pos += xlen;

//             return RNG_SUCCESS;
//         }

//         // take what's in the buffer
//         memcpy(x + offset, ctx->buffer + ctx->buffer_pos, 16 - ctx->buffer_pos);
//         xlen -= 16 - ctx->buffer_pos;
//         offset += 16 - ctx->buffer_pos;

//         AES256_ECB(ctx->key, ctx->ctr, ctx->buffer);
//         ctx->buffer_pos = 0;

//         // increment the counter
//         for (int i = 15; i >= 12; i--) {
//             if (ctx->ctr[i] == 0xff)
//                 ctx->ctr[i] = 0x00;
//             else {
//                 ctx->ctr[i]++;
//                 break;
//             }
//         }
//     }

//     return RNG_SUCCESS;
// }

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

void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string)
{
    // printf("randombytes_init test\n");
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

int randombytes(unsigned char *x, unsigned long long xlen)
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