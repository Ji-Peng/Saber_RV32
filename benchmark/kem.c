#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles.h"
#include "poly.h"
#include "rng.h"
#include "verify.h"

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
        printf("%u \t %u\n", ss_a[i], ss_b[i]);
        if (ss_a[i] != ss_b[i]) {
            printf(" ----- ERR CCA KEM ------\n");
            break;
        }
    }

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
    printf("the end\n");
    return 0;
}

void test_ntt(void)
{
    uint16_t b[SABER_L][SABER_N] = {0}, s[SABER_L][SABER_N] = {0};
    uint16_t res1[SABER_N], res2[SABER_N];
    memset(b, 0, sizeof(b));
    memset(s, 0, sizeof(s));
    memset(res1, 0, sizeof(res1));
    memset(res2, 0, sizeof(res2));

    b[0][0] = s[0][0] = 8191;
    InnerProd(b, s, res1);
    InnerProd_ntt(b, s, res2);
    for (int i = 0; i < SABER_N; i++) {
        if (res1[i] != res2[i]) {
            printf("res1[%d]:%d,res2[%d]:%d ", i, res1[i], i, res2[i]);
        }
    }
}

int main(void)
{
    // test_kem_cpa();
    // test_kem_cca();
    test_ntt();
    return 0;
}
