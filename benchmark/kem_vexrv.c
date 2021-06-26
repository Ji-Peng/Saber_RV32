#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "fips202.h"
#include "hal.h"
#include "ntt.h"
#include "poly.h"
#include "poly_mul.h"
#include "reduce.h"
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

static int TestPolyMul(void)
{
    uint16_t a[2 * SABER_N];
    // uint32_t b[SABER_N];
    uint16_t* c = a;
    uint32_t* b = (uint32_t*)a;
    int j;
    uint64_t t1, t2, sum1, sum2, sum3, sum4;
    sum1 = sum2 = sum3 = sum4 = 0;

    hal_send_str("NTT PolyBaseMul InvNTT PolyMulAcc: ");
    for (j = 0; j < NTESTS; j++) {
        t1 = hal_get_time();
        NTT(a, b);
        t2 = hal_get_time();
        sum1 += (t2 - t1);

        t1 = hal_get_time();
        PolyBaseMul((int32_t*)a, b);
        t2 = hal_get_time();
        sum2 += (t2 - t1);

        t1 = hal_get_time();
        InvNTT((int32_t*)a, b);
        t2 = hal_get_time();
        sum3 += (t2 - t1);

        t1 = hal_get_time();
        PolyMulAcc(a, (uint16_t*)b, c);
        t2 = hal_get_time();
        sum4 += (t2 - t1);
    }
    printcycles(" ", (sum1 / NTESTS));
    printcycles(" ", (sum2 / NTESTS));
    printcycles(" ", (sum3 / NTESTS));
    printcycles(" ", (sum4 / NTESTS));
    hal_send_str("\n");
    return 0;
}

static void PrintConfig(void)
{
    printcycles("SABER_L is: ", SABER_L);
    hal_send_str("Strategy: ");
#ifdef FASTGENA_SLOWMUL
    hal_send_str("FASTGENA_SLOWMUL ");
#elif defined(FASTGENA_FASTMUL)
    hal_send_str("FASTGENA_FASTMUL ");
#elif defined(SLOWGENA_FASTMUL)
    hal_send_str("SLOWGENA_FASTMUL ");
#endif

    hal_send_str("NTT: ");
#ifdef COMPLETE_NTT
    hal_send_str("COMPLETE_NTT ");
#elif defined(SEVEN_LAYER_NTT)
    hal_send_str("SEVEN_LAYER_NTT ");
#elif defined(SIX_LAYER_NTT)
    hal_send_str("SIX_LAYER_NTT ");
#elif defined(FIVE_LAYER_NTT)
    hal_send_str("FIVE_LAYER_NTT ");
#endif

#ifdef NTTASM
    hal_send_str("ASM Implementation\n");
#else
    hal_send_str("C Implementation\n");
#endif
}

int main(void)
{
    hal_setup(CLOCK_BENCHMARK);
    PrintConfig();
    TestCCA();
    // SpeedNTT();
    SpeedCCA();
    TestPolyMul();
    // SpeedCCAKP();
    // SpeedCCAEnc();
    // SpeedCCADec();
    return 0;
}
