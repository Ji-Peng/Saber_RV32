#include "rng.h"

#include <stdint.h>
#include <string.h>

#define AES_CTR_MODE

#ifdef AES_CTR_MODE

#    include "aes.h"

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

#else
void RandomBytesInit(unsigned char *entropy_input,
                     unsigned char *personalization_string)
{
    (void)entropy_input;
}

static uint32_t seed[32] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3,
                            2, 3, 8, 4, 6, 2, 6, 4, 3, 3, 8, 3, 2, 7, 9, 5};
static uint32_t in[12];
static uint32_t out[8];
static int32_t outleft = 0;

#    define ROTATE(x, b) (((x) << (b)) | ((x) >> (32 - (b))))
#    define MUSH(i, b) x = t[i] += (((x ^ seed[i]) + sum) ^ ROTATE(x, b));

static void surf(void)
{
    uint32_t t[12];
    uint32_t x;
    uint32_t sum = 0;
    int32_t r;
    int32_t i;
    int32_t loop;

    for (i = 0; i < 12; ++i) {
        t[i] = in[i] ^ seed[12 + i];
    }
    for (i = 0; i < 8; ++i) {
        out[i] = seed[24 + i];
    }
    x = t[11];
    for (loop = 0; loop < 2; ++loop) {
        for (r = 0; r < 16; ++r) {
            sum += 0x9e3779b9;
            MUSH(0, 5)
            MUSH(1, 7)
            MUSH(2, 9)
            MUSH(3, 13)
            MUSH(4, 5)
            MUSH(5, 7)
            MUSH(6, 9)
            MUSH(7, 13)
            MUSH(8, 5)
            MUSH(9, 7)
            MUSH(10, 9)
            MUSH(11, 13)
        }
        for (i = 0; i < 8; ++i) {
            out[i] ^= t[i + 4];
        }
    }
}

int RandomBytes(unsigned char *buf, unsigned long long xlen)
{
    while (xlen > 0) {
        if (!outleft) {
            if (!++in[0]) {
                if (!++in[1]) {
                    if (!++in[2]) {
                        ++in[3];
                    }
                }
            }
            surf();
            outleft = 8;
        }
        *buf = (uint8_t)out[--outleft];
        ++buf;
        --xlen;
    }
    return 0;
}

#endif