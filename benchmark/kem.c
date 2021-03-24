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
#include "ntt.h"
#include "poly.h"
#include "poly_mul.h"
#include "reduce.h"
#include "rng.h"
#include "verify.h"

static int TestCCA(void);
static int TestCPA(void);
static int SpeedCPA(void);
static int SpeedCCA(void);

#define NTESTS 1000

static void DisableWatchDog(void)
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
    printf("crypto_kem_keypair  %s\n", ullu(sum1 / NTESTS));
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
    printf("crypto_kem_enc      %s\n", ullu(sum2 / NTESTS));
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
    printf("crypto_kem_dec      %s\n", ullu(sum3 / NTESTS));
    return 0;
}

static int TestPolyMul(void)
{
    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
    uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
    uint8_t ciphertext[SABER_BYTES_CCA_DEC];

    uint16_t a[2 * SABER_N];
    uint16_t b[SABER_L][SABER_N];
    uint16_t c[SABER_N];
    int j;
    uint64_t t1, t2, sum1, sum2, sum3, sum4;
    sum1 = sum2 = sum3 = sum4 = 0;

    for (j = 0; j < NTESTS; j++) {
        t1 = cpucycles();
        MatrixVectorMulKP(seed_A, seed_s, sk, b);
        t2 = cpucycles();
        sum1 += (t2 - t1);

        t1 = cpucycles();
        MatrixVectorMulEnc(seed_A, b, ciphertext);
        t2 = cpucycles();
        sum2 += (t2 - t1);

        t1 = cpucycles();
        PolyMulAcc(a, (uint16_t *)b, c);
        t2 = cpucycles();
        sum4 += (t2 - t1);
    }
    printf("MatrixVectorMulKP   %s\n", ullu(sum1 / NTESTS));
    printf("MatrixVectorMulEnc  %s\n", ullu(sum2 / NTESTS));
    printf("InnerProdInTime     %s\n", ullu(sum3 / NTESTS));
    printf("PolyMulAcc          %s\n", ullu(sum4 / NTESTS));
    return 0;
}

// static int TestGen(void)
// {
//     uint16_t poly[SABER_N];
//     uint8_t seed[SABER_SEEDBYTES];
//     int i, j, k;
//     uint64_t t1, t2, sum1, sum2;
//     sum1 = sum2 = 0;

//     for (k = 0; k < NTESTS; k++) {
//         for (i = 0; i < SABER_L; i++) {
//             for (j = 0; j < SABER_L; j++) {
//                 t1 = cpucycles();
//                 GenAInTime(poly, seed, 1 - i - j);
//                 t2 = cpucycles();
//                 sum1 += (t2 - t1);
//             }
//             t1 = cpucycles();
//             GenSInTime(poly, seed, i);
//             t2 = cpucycles();
//             sum2 += (t2 - t1);
//         }
//     }
//     printf("GenAInTime*9        %s\n", ullu(sum1 / NTESTS));
//     printf("GenSInTime          %s\n", ullu(sum2 / NTESTS));
//     return 0;
// }

static int TestKeccak(void)
{
    uint64_t keccak_state[25];
    uint8_t seed[32], buf[168];
    int i;
    uint64_t t1, t2, sum1, sum2;
    sum1 = sum2 = 0;

    for (i = 0; i < 25; i++) {
        keccak_state[i] = 0;
    }

    for (i = 0; i < NTESTS; i++) {
        t1 = cpucycles();
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, 32, 0x1F);
        t2 = cpucycles();
        sum1 += (t2 - t1);

        t1 = cpucycles();
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        t2 = cpucycles();
        sum2 += (t2 - t1);
    }

    printf("keccak_absorb       %s\n", ullu(sum1 / NTESTS));
    printf("keccak_squeeze      %s\n", ullu(sum2 / NTESTS));
    return 0;
}

static int TestNTT(void)
{
    int i;
    uint64_t t1, t2, sum1;
    uint16_t in[SABER_N];
    int32_t out[SABER_N];
    sum1 = 0;

    for (i = 0; i < NTESTS; i++) {
        t1 = cpucycles();
        NTT(in, out);
        t2 = cpucycles();
        sum1 += (t2 - t1);
    }
    printf("NTT                 %s\n", ullu(sum1 / NTESTS));
    return 0;
}

#endif

static void TestNTTRange(void)
{
    int i;
    uint16_t a[SABER_N * 2], s[SABER_N], r[SABER_N] = {0};
    for (i = 0; i < SABER_N; i++) {
        s[i] = 4;
        a[i] = 4095;
    }
    PolyMulAcc(a, s, r);
    for (i = 0; i < SABER_N; i++) {
        printf("%hd ", r[i] & 0x1fff);
    }
}

int main(void)
{
    DisableWatchDog();
    // TestCPA();
    // TestCCA();
#ifndef HOST
    // SpeedCPA();
    // SpeedCCAKP();
    // SpeedCCAEnc();
    // SpeedCCADec();
    // SpeedCCA();
    // TestPolyMul();
    // TestGen();
    // TestKeccak();
    // TestNTT();
#endif
    TestNTTRange();
    return 0;
}
