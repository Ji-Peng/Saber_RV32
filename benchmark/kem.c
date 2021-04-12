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
#ifndef HOST
#    include "metal/watchdog.h"
#endif
#include "poly_mul.h"

#define NTESTS 1

static int test_kem_cca(void);
static int test_kem_cpa(void);
static int speed_cpa(void);
static int speed_cca(void);

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

    for (i = 0; i < 32; i++) {
        if (message1[i] != message2[i]) {
            printf("i=%d, %d, %d\n", i, message1[i], message2[i]);
            printf("ERROR\n");
            break;
        }
    }
    printf("the end\n");
    return 0;
}

static int speed_cpa(void)
{
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t ct[SABER_BYTES_CCA_DEC];
    // for saving memory
    // uint8_t *pk = sk;
    // uint8_t *ct = sk;
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
    // uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    // uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];
    uint8_t *pk = ct, *sk = ct;

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
        printf("keypair start\n");
        crypto_kem_keypair(pk, sk);
        t2 = cpucycles();
        sum1 += (t2 - t1);
        printf("keypair end\n");
        // Key-Encapsulation call; input: pk; output: ciphertext c,
        // shared-secret ss_a;
        t1 = cpucycles();
        crypto_kem_enc(ct, ss_a, pk);
        t2 = cpucycles();
        sum2 += (t2 - t1);
        printf("enc end\n");
        // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
        t1 = cpucycles();
        crypto_kem_dec(ss_b, ct, sk);
        t2 = cpucycles();
        sum3 += (t2 - t1);
        printf("dec end\n");
    }
    printf("????\n");
    printf("crypto_kem_keypair/enc/dec/overall:");
    printf("%s/", ullu(sum1 / NTESTS));
    printf("%s/", ullu(sum2 / NTESTS));
    printf("%s/", ullu(sum3 / NTESTS));
    printf("%s\n", ullu((sum1 + sum2 + sum3) / NTESTS));
    return 0;
}

static int speed_polmul(void)
{
    uint16_t a[SABER_N], b[SABER_N], c[SABER_N];
    int j;
    uint64_t t1, t2, sum;
    sum = 0;
    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        pol_mul(a, b, c);
        t2 = cpucycles();
        sum += (t2 - t1);
    }
    printf("pol_mul %s\n", ullu(sum / NTESTS));
    return 0;
}

static int speed_MatrixVectorMul(void)
{
    uint8_t seed[SABER_SEEDBYTES];
    uint16_t skpv[SABER_K][SABER_N];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    int j;
    uint64_t t1, t2, sum1, sum2, sum3;
    sum1 = sum2 = sum3 = 0;
    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        MatrixVectorMul_keypair(seed, skpv, skpv, SABER_Q - 1);
        t2 = cpucycles();
        sum1 += (t2 - t1);

        t1 = cpucycles();
        MatrixVectorMul_encryption(seed, skpv, ct, SABER_Q - 1);
        t2 = cpucycles();
        sum2 += (t2 - t1);

        t1 = cpucycles();
        VectorMul(ct, skpv, skpv[SABER_K - 1]);
        t2 = cpucycles();
        sum3 += (t2 - t1);
    }
    printf("MatrixVectorMul_keypair    %s\n", ullu(sum1 / NTESTS));
    printf("MatrixVectorMul_encryption %s\n", ullu(sum2 / NTESTS));
    printf("VectorMul                  %s\n", ullu(sum3 / NTESTS));
    return 0;
}

static int speed_GenInTime(void)
{
    uint16_t temp_ar[SABER_N];
    uint64_t t1, t2, sum1;
    int i, j, k;
    uint8_t seed[SABER_SEEDBYTES];
    sum1 = 0;

    for (k = 0; k < NTESTS; k++) {
        for (i = 0; i < SABER_K; i++) {
            for (j = 0; j < SABER_K; j++) {
                t1 = cpucycles();
                GenMatrix_poly(temp_ar, seed, i + j);
                t2 = cpucycles();
                sum1 += (t2 - t1);
            }
        }
    }
    printf("GenMatrix_poly*9 %s\n", ullu(sum1 / NTESTS));
    return 0;
}

static int speed_GenS(void)
{
    uint16_t r[SABER_K][SABER_N];
    uint8_t seed[SABER_SEEDBYTES];
    uint64_t t1, t2, sum1;
    int j;
    sum1 = 0;
    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        GenSecret(r, seed);
        t2 = cpucycles();
        sum1 += (t2 - t1);
    }
    printf("GenSecret %s\n", ullu(sum1 / NTESTS));
    return 0;
}

static int SpeedCCA(void)
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
    printf("crypto_kem_keypair/enc/dec/all: ");
    printf("%s/", ullu(sum1 / NTESTS));
    printf("%s/", ullu(sum2 / NTESTS));
    printf("%s/", ullu(sum3 / NTESTS));
    printf("%s\n", ullu((sum1 + sum2 + sum3) / NTESTS));
    return 0;
}

static int SpeedCCAKP(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];

    unsigned char entropy_input[48];

    int i, j;
    uint64_t t1, t2, sum1;
    sum1 = 0;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    for (j = 0; j < NTESTS; j++) {
        // Generation of secret key sk and public key pk pair
        t1 = cpucycles();
        crypto_kem_keypair(pk, sk);
        t2 = cpucycles();
        sum1 += (t2 - t1);
    }
    printf("crypto_kem_keypair/enc/dec: %s/", ullu(sum1 / NTESTS));
    return 0;
}

static int SpeedCCAEnc(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i, j;
    uint64_t t1, t2, sum2;
    sum2 = 0;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    for (j = 0; j < NTESTS; j++) {
        // Key-Encapsulation call; input: pk; output: ciphertext c,
        // shared-secret ss_a;
        t1 = cpucycles();
        crypto_kem_enc(ct, ss_a, pk);
        t2 = cpucycles();
        sum2 += (t2 - t1);
    }
    printf("%s/", ullu(sum2 / NTESTS));
    return 0;
}

static int SpeedCCADec(void)
{
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_b[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i, j;
    uint64_t t1, t2, sum3;
    sum3 = 0;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    randombytes_init(entropy_input, NULL);

    for (j = 0; j < NTESTS; j++) {
        // Key-Decapsulation call; input: sk, c; output: shared-secret ss_b;
        t1 = cpucycles();
        crypto_kem_dec(ss_b, ct, sk);
        t2 = cpucycles();
        sum3 += (t2 - t1);
    }
    printf("%s\n", ullu(sum3 / NTESTS));
    return 0;
}

int main(void)
{
    disable_watchdog();
    // test_kem_cpa();
    // test_kem_cca();
    // speed_cpa();
    // speed_cca();
    // speed_polmul();
    // speed_MatrixVectorMul();
    // speed_GenInTime();
    // speed_GenS();
    SpeedCCAKP();
    SpeedCCAEnc();
    SpeedCCADec();
    return 0;
}
