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
    printf("keypair ok\n");

    // Key-Encapsulation call; input: pk; output: ciphertext c, shared-secret
    // ss_a;
    crypto_kem_enc(ct, ss_a, pk);
    printf("enc ok\n");

    // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
    crypto_kem_dec(ss_b, ct, sk);
    printf("dec ok\n");

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

static int test_kem_keypair(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    unsigned char entropy_input[48];
    int i;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i;
    randombytes_init(entropy_input, NULL);
    crypto_kem_keypair(pk, sk);
}

int main(void)
{
    printf("hello world\n");
    test_kem_cca();
    // test_kem_keypair();
    printf("congratulation\n");
    return 0;
}
