#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles.h"
#include "ntt.h"
#include "poly.h"
#include "reduce.h"
#include "rng.h"
#include "verify.h"

static int test_kem_cca(void);
static int test_kem_cpa(void);
static void test_ntt(void);
static void test_ntt_self(void);

static int test_kem_cca(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i;
    randombytes_init(entropy_input, NULL);

    // Generation of secret key sk and public key pk pair
    crypto_kem_keypair(pk, sk);

    // Key-Encapsulation call; input: pk; output: ciphertext c, shared-secret
    // ss_a;
    crypto_kem_enc(ct, ss_a, pk);

    // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
    crypto_kem_dec(ss_b, ct, sk);

    // Functional verification: check if ss_a == ss_b?
    for (i = 0; i < SABER_KEYBYTES; i++) {
        // printf("%u \t %u\n", ss_a[i], ss_b[i]);
        if (ss_a[i] != ss_b[i]) {
            printf(" ----- ERR CCA KEM ------\n");
            break;
        }
    }
    printf("test_kem_cca end\n");
    return 0;
}

static int test_kem_cpa(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t message1[64], message2[64];
    uint8_t noiseseed[32];
    uint8_t entropy_input[48];
    int i;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i;
    randombytes_init(entropy_input, NULL);

    memset(message1, 0, sizeof(message1));
    memset(message2, 0, sizeof(message2));
    memset(noiseseed, 0, sizeof(noiseseed));

    for (i = 0; i < 32; i++) {
        message1[i] = noiseseed[i] = i;
    }

    indcpa_kem_keypair(pk, sk);
    indcpa_kem_enc(message1, noiseseed, pk, ct);
    indcpa_kem_dec(sk, ct, message2);

    for (i = 0; i < 64; i++) {
        if (message1[i] != message2[i]) {
            printf("i=%d, %d, %d\n", i, message1[i], message2[i]);
            printf("ERROR\n");
            break;
        }
    }
    printf("test_kem_cpa end\n");
    return 0;
}

static void test_ntt(void)
{
    uint16_t b[SABER_L][SABER_N] = {0}, s[SABER_L][SABER_N] = {0};
    uint16_t res1[SABER_N], res2[SABER_N];
    memset(b, 0, sizeof(b));
    memset(s, 0, sizeof(s));
    memset(res1, 0, sizeof(res1));
    memset(res2, 0, sizeof(res2));

    // b[0][0] = s[0][0] = 512;
    for (int i = 0; i < SABER_L; i++) {
        for (int j = 0; j < SABER_N; j++) {
            b[i][j] = j;
            s[i][j] = j;
        }
    }
    InnerProd(b, s, res1);
    InnerProd_ntt((int16_t(*)[256])b, (int16_t(*)[256])s, (int16_t *)res2);
    for (int i = 0; i < SABER_N; i++) {
        if ((res1[i] & 0x1fff) != (res2[i] & 0x1fff)) {
            printf("res1[%d]:%d,res2[%d]:%d ", i, res1[i], i, res2[i]);
        }
    }
    printf("test_ntt end\n");
}

static void test_ntt_self(void)
{
    int16_t res1[SABER_N];
    int32_t res2[SABER_N], res3[SABER_N], j;
    memset(res1, 0, sizeof(res1));
    memset(res2, 0, sizeof(res2));
    memset(res3, 0, sizeof(res3));
    // res1[0] = 1;
    for (int i = 0; i < SABER_N; i++) {
        res1[i] = i;
    }
    ntt(res1, (int32_t *)res2);
    invntt((int32_t *)res2, (int32_t *)res3);
    for (j = 0; j < SABER_N; j++) {
        printf("%d ", res3[j]);
    }
    printf("test_ntt_self end\n");
}

int main(void)
{
    test_kem_cpa();
    test_kem_cca();
    // test_ntt();
    // test_ntt_self();
    return 0;
}
