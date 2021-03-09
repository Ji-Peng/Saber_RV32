#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles.h"
#ifndef HOST
#    include "metal/watchdog.h"
#endif
#include "ntt.h"
#include "poly.h"
#include "reduce.h"
#include "rng.h"
#include "verify.h"

static int test_kem_cca(void);
static int test_kem_cpa(void);
static void test_ntt(void);
static void test_ntt_self(void);

#define NTEST 1

static void disable_watchdog(void)
{
#ifndef HOST
    int result = 1;
    struct metal_watchdog *wdog;
    wdog = metal_watchdog_get_device(0);
    result = metal_watchdog_run(wdog, METAL_WATCHDOG_STOP);
    if (result != 0) {
        printf("watchdog disable failed\n");
    } else if (result == 0) {
        printf("watchdog disable success\n");
    } else {
        printf("unknown return value %d\n", result);
    }
#endif
}

static int test_kem_cca(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i, j;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    for (j = 0; j < NTEST; j++) {
        // Generation of secret key sk and public key pk pair
        crypto_kem_keypair(pk, sk);

        // Key-Encapsulation call; input: pk; output: ciphertext c,
        // shared-secret ss_a;
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
        if (i != SABER_KEYBYTES) {
            break;
        }
    }
    printf("test_kem_cca end\n");
    return 0;
}

static int test_kem_cpa(void)
{
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t ct[SABER_BYTES_CCA_DEC];
    uint8_t message1[SABER_KEYBYTES], message2[SABER_KEYBYTES];
    uint8_t noiseseed[32];
    uint8_t entropy_input[48];
    int i, j;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    for (j = 0; j < NTEST; j++) {
        memset(message1, 0, sizeof(message1));
        memset(message2, 0, sizeof(message2));
        memset(noiseseed, 0, sizeof(noiseseed));

        for (i = 0; i < 32; i++) {
            message1[i] = noiseseed[i] = i;
        }

        printf("1234\n");
        indcpa_kem_keypair(pk, sk);
        printf("2345\n");
        indcpa_kem_enc(message1, noiseseed, pk, ct);
        printf("3456\n");
        indcpa_kem_dec(sk, ct, message2);
        printf("4567\n");

        for (i = 0; i < SABER_KEYBYTES; i++) {
            if (message1[i] != message2[i]) {
                printf("i=%d, %d, %d\n", i, message1[i], message2[i]);
                printf("ERROR\n");
                break;
            }
        }
        if (i != SABER_KEYBYTES) {
            break;
        }
    }
    printf("test_kem_cpa end\n");
    return 0;
}

int main(void)
{
    disable_watchdog();
    test_kem_cpa();
    test_kem_cca();
    return 0;
}
