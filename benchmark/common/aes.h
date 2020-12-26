#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stdlib.h>

#define AES128_KEYBYTES 16
#define AES192_KEYBYTES 24
#define AES256_KEYBYTES 32
#define AESCTR_NONCEBYTES 12
#define AES_BLOCKBYTES 16

// We've put these states on the heap to make sure ctx_release is used.
#define PQC_AES128_STATESIZE 88
typedef struct {
    uint64_t* sk_exp;
} aes128ctx;

#define PQC_AES192_STATESIZE 104
typedef struct {
    uint64_t*  sk_exp;
} aes192ctx;

#define PQC_AES256_STATESIZE 120
typedef struct {
    uint64_t* sk_exp;
} aes256ctx;

/** Initializes the context **/
void aes256_ecb_keyexp(aes256ctx *r, const unsigned char *key);

void aes256_ctr_keyexp(aes256ctx *r, const unsigned char *key);

void aes256_ecb(unsigned char *out, const unsigned char *in, size_t nblocks, const aes256ctx *ctx);

void aes256_ctr(unsigned char *out, size_t outlen, const unsigned char *iv, const aes256ctx *ctx);

/** Frees the context **/
void aes256_ctx_release(aes256ctx *r);


#endif