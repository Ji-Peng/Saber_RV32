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
#include "fips202.h"
#include "poly.h"
#include "poly_mul.h"
#include "rng.h"
#include "verify.h"

static int TestCCA(void);
static int TestCPA(void);
static int SpeedCPA(void);
static int SpeedCCA(void);

static char* ullu(uint64_t val)
{
    static char buf[21] = {0};
    buf[20] = 0;
    char* out = &buf[19];
    uint64_t hval = val;
    unsigned int hbase = 10;

    do {
        *out = "0123456789"[hval % hbase];
        --out;
        hval /= hbase;
    } while (hval);

    return ++out;
}

#ifdef HOST
#    define NTESTS 1000
#else
#    define NTESTS 10
#endif
static void DisableWatchDog(void)
{
#ifndef HOST
    int result = 1;
    struct metal_watchdog* wdog;
    wdog = metal_watchdog_get_device(0);
    result = metal_watchdog_run(wdog, METAL_WATCHDOG_STOP);
    if (result != 0) {
        printf("watchdog disable failed\n");
    } else if (result == 0) {
        // printf("watchdog disable success\n");
    } else {
        printf("unknown return value %d\n", result);
    }
#endif
}

static int TestCPA(void)
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
    RandomBytesInit(entropy_input, NULL);
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
    return 0;
}

static int TestCCA(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];

    unsigned char entropy_input[48];

    int i, j;

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    RandomBytesInit(entropy_input, NULL);
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
    return 0;
}

#ifndef HOST

static int SpeedCPA(void)
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
    RandomBytesInit(entropy_input, NULL);

    memset(message1, 0, sizeof(message1));
    memset(message2, 0, sizeof(message2));
    memset(noiseseed, 0, sizeof(noiseseed));
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
    printf("indcpa_kem_keypair/enc/dec/all: ");
    printf("%s/", ullu(sum1 / NTESTS));
    printf("%s/", ullu(sum2 / NTESTS));
    printf("%s/", ullu(sum3 / NTESTS));
    printf("%s\n", ullu((sum1 + sum2 + sum3) / NTESTS));
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
    RandomBytesInit(entropy_input, NULL);

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
    RandomBytesInit(entropy_input, NULL);

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
    RandomBytesInit(entropy_input, NULL);

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
    RandomBytesInit(entropy_input, NULL);

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

static int SpeedMatrixVector()
{
    uint16_t a[SABER_K][SABER_K][SABER_N];
    // uint16_t skpv[SABER_K][SABER_N];
    // uint32_t bw_ar[SABER_K][7][NUM_POLY_MID][N_SB_16];
    uint16_t* skpv = a;
    uint32_t* bw_ar = a;
    uint16_t* res = a;
    int i, j;
    uint64_t t1, t2, sum = 0;

    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        // pre-computation B
        for (i = 0; i < SABER_K; i++)
            evaluation_single((const uint16_t*)skpv[i], bw_ar[i]);
        MatrixVectorMul(a, bw_ar, res, SABER_Q - 1, 1);
        t2 = cpucycles();
        sum += (t2 - t1);
    }
    printf("MatrixVectorMul %s\n", ullu(sum / NTESTS));
}

#endif
static void PrintConfig(void)
{
    printf("SABER_L is %d\n", SABER_K);
    printf("Strategy: ");
#ifdef FASTGENA_SLOWMUL
    printf("FASTGENA_SLOWMUL\n");
#elif defined(FASTGENA_FASTMUL)
    printf("FASTGENA_FASTMUL\n");
#elif defined(SLOWGENA_FASTMUL)
    printf("SLOWGENA_FASTMUL\n");
#endif

    printf("NTT: ");
#ifdef COMPLETE_NTT
    printf("COMPLETE_NTT ");
#elif defined(SEVEN_LAYER_NTT)
    printf("SEVEN_LAYER_NTT ");
#elif defined(SIX_LAYER_NTT)
    printf("SIX_LAYER_NTT ");
#endif

#ifdef NTTASM
    printf("ASM Implementation\n");
#else
    printf("C Implementation\n");
#endif
}

int main(void)
{
    PrintConfig();
    DisableWatchDog();
    // TestCPA();
    // TestCCA();
#ifndef HOST
    // SpeedCPA();
    // TestPolyMul();
    // SpeedCPA();
    // SpeedCCA();
    // TestCenR();
    // TestNTT();
    // SpeedCCAKP();
    // SpeedCCAEnc();
    // SpeedCCADec();
    // TestGen();
    // TestKeccak();
    SpeedMatrixVector();
#endif
    // TestNTTRange();
    return 0;
}
