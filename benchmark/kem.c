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
static int speed_cpa(void);
static int speed_cca(void);

#define NTESTS 1

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
    printf("-----------TEST CPA CORRECTNESS-------------\n");
    for (j = 0; j < NTESTS; j++) {
        memset(message1, 0, sizeof(message1));
        memset(message2, 0, sizeof(message2));
        memset(noiseseed, 0, sizeof(noiseseed));

        for (i = 0; i < 32; i++) {
            message1[i] = noiseseed[i] = i;
        }

        indcpa_kem_keypair(pk, sk);
        indcpa_kem_enc(message1, noiseseed, pk, ct);
        indcpa_kem_dec(sk, ct, message2);

        for (i = 0; i < SABER_KEYBYTES; i++) {
            if (message1[i] != message2[i]) {
                printf("i=%d, %d, %d\n", i, message1[i], message2[i]);
                printf(" ----- ERROR CPA ------\n");
                break;
            }
        }
        if (i != SABER_KEYBYTES) {
            break;
        }
    }
    if (i == SABER_KEYBYTES) {
        printf("kem_cpa right!!!\n");
    } else {
        printf("kem_cpa ERROR\n");
    }
    printf("-----------TEST CPA CORRECTNESS-------------\n");
    return 0;
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
    printf("-----------TEST CCA CORRECTNESS-------------\n");
    for (j = 0; j < NTESTS; j++) {
        // Generation of secret key sk and public key pk pair
        crypto_kem_keypair(pk, sk);
        // Key-Encapsulation call; input: pk; output: ciphertext c,
        // shared-secret ss_a;
        crypto_kem_enc(ct, ss_a, pk);
        // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
        crypto_kem_dec(ss_b, ct, sk);
        // Functional verification: check if ss_a == ss_b?
        for (i = 0; i < SABER_KEYBYTES; i++) {
            if (ss_a[i] != ss_b[i]) {
                printf("i=%d, %d, %d\n", i, ss_a[i], ss_b[i]);
                printf(" ----- ERR CCA KEM ------\n");
                break;
            }
        }
        if (i != SABER_KEYBYTES) {
            break;
        }
    }
    if (i == SABER_KEYBYTES) {
        printf("kem_cca right!!!\n");
    } else {
        printf("kem_cca ERROR\n");
    }
    printf("-----------TEST CCA CORRECTNESS-------------\n");
    return 0;
}

static int speed_cpa(void)
{
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t ct[SABER_BYTES_CCA_DEC];
    uint8_t message1[SABER_KEYBYTES], message2[SABER_KEYBYTES];
    uint8_t noiseseed[32];
    uint8_t entropy_input[48];
    int i, j;
    uint64_t t1, t2, sum1, sum2, sum3;
    sum1 = sum2 = sum3 = 0;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    memset(message1, 0, sizeof(message1));
    memset(message2, 0, sizeof(message2));
    memset(noiseseed, 0, sizeof(noiseseed));
    printf("-----------TEST CPA SPEED-------------\n");
    for (j = 0; j < NTESTS; j++) {
        for (i = 0; i < 32; i++) {
            message1[i] = noiseseed[i] = i;
        }
        t1 = cpucycles();
        indcpa_kem_keypair(pk, sk);
        t2 = cpucycles();
        sum1 += (t2 - t1);
        t1 = cpucycles();
        indcpa_kem_enc(message1, noiseseed, pk, ct);
        t2 = cpucycles();
        sum2 += (t2 - t1);
        t1 = cpucycles();
        indcpa_kem_dec(sk, ct, message2);
        t2 = cpucycles();
        sum3 += (t2 - t1);
    }
    printf("indcpa_kem_keypair  %s\n", ullu(sum1 / NTESTS));
    printf("indcpa_kem_enc      %s\n", ullu(sum2 / NTESTS));
    printf("indcpa_kem_dec      %s\n", ullu(sum3 / NTESTS));
    printf("overall             %s\n", ullu((sum1 + sum2 + sum3) / NTESTS));
    printf("NTESTS              %d\n", NTESTS);
    printf("-----------TEST CPA SPEED-------------\n");
    return 0;
}

static int speed_cca(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i, j;
    uint64_t t1, t2, sum1, sum2, sum3;
    sum1 = sum2 = sum3 = 0;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    printf("-----------TEST CCA SPEED-------------\n");
    for (j = 0; j < NTESTS; j++) {
        // Generation of secret key sk and public key pk pair
        t1 = cpucycles();
        crypto_kem_keypair(pk, sk);
        t2 = cpucycles();
        sum1 += (t2 - t1);
        // Key-Encapsulation call; input: pk; output: ciphertext c,
        // shared-secret ss_a;
        t1 = cpucycles();
        crypto_kem_enc(ct, ss_a, pk);
        t2 = cpucycles();
        sum2 += (t2 - t1);
        // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
        t1 = cpucycles();
        crypto_kem_dec(ss_b, ct, sk);
        t2 = cpucycles();
        sum3 += (t2 - t1);
    }
    printf("crypto_kem_keypair  %s\n", ullu(sum1 / NTESTS));
    printf("crypto_kem_enc      %s\n", ullu(sum2 / NTESTS));
    printf("crypto_kem_dec      %s\n", ullu(sum3 / NTESTS));
    printf("overall             %s\n", ullu((sum1 + sum2 + sum3) / NTESTS));
    printf("NTESTS              %d\n", NTESTS);
    printf("-----------TEST CCA SPEED-------------\n");
    return 0;
}

static int test_polmul(void)
{
    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t ciphertext[SABER_BYTES_CCA_DEC];

    uint16_t a[2 * SABER_N];
    uint16_t b[SABER_L][SABER_N];
    int j;
    uint64_t t1, t2, sum1, sum2, sum3, sum4;
    sum1 = sum2 = sum3 = sum4 = 0;

    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        MatrixVectorMulKP_ntt(seed_A, seed_s, sk, b);
        t2 = cpucycles();
        sum1 += (t2 - t1);

        t1 = cpucycles();
        MatrixVectorMulEnc_ntt(seed_A, b, ciphertext);
        t2 = cpucycles();
        sum2 += (t2 - t1);

        t1 = cpucycles();
        InnerProdInTime_ntt(pk, b, a);
        t2 = cpucycles();
        sum3 += (t2 - t1);

        t1 = cpucycles();
        poly_mul_acc_ntt(a, b, b);
        t2 = cpucycles();
        sum4 += (t2 - t1);
    }
}

int main(void)
{
    disable_watchdog();
    // test_kem_cpa();
    // test_kem_cca();
    // speed_cpa();
    // speed_cca();
    test_polmul();
    return 0;
}
