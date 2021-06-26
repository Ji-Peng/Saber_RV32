#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "fips202.h"
#include "hal.h"
#include "poly.h"
#include "poly_mul.h"
#include "rng.h"
#include "verify.h"

#define NTESTS 1

static void printcycles(const char* s, unsigned long long c)
{
    char outs[32];
    hal_send_str(s);
    snprintf(outs, sizeof(outs), "%llu ", c);
    hal_send_str(outs);
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
    hal_send_str("-----------TEST CCA CORRECTNESS-------------\n");
    for (j = 0; j < 1; j++) {
        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, ss_a, pk);
        crypto_kem_dec(ss_b, ct, sk);
        for (i = 0; i < SABER_KEYBYTES; i++) {
            if (ss_a[i] != ss_b[i]) {
                hal_send_str("CCA KEM ERROR\n");
                break;
            }
        }
        if (i != SABER_KEYBYTES) {
            break;
        }
    }
    if (i == SABER_KEYBYTES) {
        hal_send_str("CCA KEM RIGHT\n");
    }
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
        t1 = hal_get_time();
        crypto_kem_keypair(pk, sk);
        t2 = hal_get_time();
        sum1 += (t2 - t1);
        t1 = hal_get_time();
        crypto_kem_enc(ct, ss_a, pk);
        t2 = hal_get_time();
        sum2 += (t2 - t1);
        t1 = hal_get_time();
        crypto_kem_dec(ss_b, ct, sk);
        t2 = hal_get_time();
        sum3 += (t2 - t1);
    }
    hal_send_str("crypto_kem_keypair,enc,dec,all: ");
    printcycles("", sum1 / NTESTS);
    printcycles("", sum2 / NTESTS);
    printcycles("", sum3 / NTESTS);
    printcycles("", (sum1 + sum2 + sum3) / NTESTS);
    hal_send_str("\n");

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
        t1 = hal_get_time();
        crypto_kem_keypair(pk, sk);
        t2 = hal_get_time();
        sum1 += (t2 - t1);
    }
    hal_send_str("crypto_kem_keypair enc dec: ");
    printcycles("", (sum1 / NTESTS));
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
        t1 = hal_get_time();
        crypto_kem_enc(ct, ss_a, pk);
        t2 = hal_get_time();
        sum2 += (t2 - t1);
    }
    printcycles(" ", (sum2 / NTESTS));
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
        t1 = hal_get_time();
        crypto_kem_dec(ss_b, ct, sk);
        t2 = hal_get_time();
        sum3 += (t2 - t1);
    }
    printcycles(" ", (sum3 / NTESTS));
    hal_send_str("\n");
    return 0;
}

static int SpeedPolyMul()
{
    uint16_t a[SABER_N], b[SABER_N], res[SABER_N * 2];
    uint64_t t1, t2, sum;
    int j;

    for (j = 0; j < NTESTS; j++) {
        t1 = hal_get_time();
        pol_mul(a, b, res, 8192, 256);
        t2 = hal_get_time();
        sum += (t2 - t1);
    }
    printcycles("pol mul: ", (sum / NTESTS));
    hal_send_str("\n");
}

int main(void)
{
    hal_setup(CLOCK_BENCHMARK);
    SpeedPolyMul();
    TestCCA();
    SpeedCCA();
    // SpeedCCAKP();
    // SpeedCCAEnc();
    // SpeedCCADec();
    return 0;
}
