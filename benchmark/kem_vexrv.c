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

#define NTESTS 100

static void printcycles(const char* s, unsigned long long c)
{
    char outs[32];
    hal_send_str(s);
    snprintf(outs, sizeof(outs), "%llu\n", c);
    hal_send_str(outs);
}

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

#ifndef HOST

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
    hal_send_str("crypto_kem_keypair/enc/dec/all: ");
    hal_send_str(ullu(sum1 / (NTESTS)));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / (NTESTS)));
    hal_send_str("/");
    hal_send_str(ullu(sum3 / (NTESTS)));
    hal_send_str("/");
    hal_send_str(ullu((sum1 + sum2 + sum3) / (NTESTS)));
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
    hal_send_str("crypto_kem_keypair/enc/dec: ");
    hal_send_str(ullu(sum1 / NTESTS));
    hal_send_str("/");
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
    hal_send_str(ullu(sum2 / NTESTS));
    hal_send_str("/");
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
    hal_send_str(ullu(sum3 / NTESTS));
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

    hal_send_str("NTT/PolyBaseMul/InvNTT/PolyMulAcc:");
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
    hal_send_str(ullu(sum1 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum3 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum4 / NTESTS));
    hal_send_str("\n");
    return 0;
}

static int TestGen(void)
{
    uint16_t s[3][SABER_N];
    uint16_t* poly = (uint16_t*)s;
    uint8_t seed[SABER_SEEDBYTES];
    int i, j, k;
    uint64_t t1, t2, t3, sum1, sum2, sum3;
    sum1 = sum2 = sum3 = 0;

    for (k = 0; k < 100; k++) {
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                t1 = hal_get_time();
#    if defined(FASTGENA_SLOWMUL) || defined(FASTGENA_FASTMUL)
                GenAInTime(poly, seed, 1 - i - j);
#    elif defined(SLOWGENA_FASTMUL)
                GenAInTime(poly, seed, i, j);
#    endif
                t2 = hal_get_time();
                sum1 += (t2 - t1);
            }
            // t1 = hal_get_time();
            // GenSInTime(poly, seed, i);
            // t2 = hal_get_time();
            // sum2 += (t2 - t1);
        }
        // t1 = hal_get_time();
        // GenSecret(s, seed);
        // t2 = hal_get_time();
        // sum3 += (t2 - t1);
    }
    hal_send_str("GenAInTime*9 / GenSInTime*3:");
    hal_send_str(ullu(sum1 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / NTESTS));
    hal_send_str("\n");
    hal_send_str("GenSecret: ");
    hal_send_str(ullu(sum3 / NTESTS));
    hal_send_str("\n");
    return 0;
}

static int TestKeccak(void)
{
    uint64_t keccak_state[25];
    uint8_t seed[32], buf[168];
    int i;
    uint64_t t1, t2, sum1, sum2, sum3, sum4;
    sum1 = sum2 = sum3 = sum4 = 0;
    uint8_t entropy_input[48];

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    RandomBytesInit(entropy_input, NULL);

    for (i = 0; i < 25; i++) {
        keccak_state[i] = 0;
    }

    for (i = 0; i < NTESTS; i++) {
        t1 = hal_get_time();
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, 32, 0x1F);
        t2 = hal_get_time();
        sum1 += (t2 - t1);

        t1 = hal_get_time();
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        t2 = hal_get_time();
        sum2 += (t2 - t1);

        t1 = hal_get_time();
        RandomBytes(seed, SABER_SEEDBYTES);
        t2 = hal_get_time();
        sum3 += (t2 - t1);

        t1 = hal_get_time();
        shake128(seed, SABER_SEEDBYTES, seed, SABER_SEEDBYTES);
        t2 = hal_get_time();
        sum4 += (t2 - t1);
    }

    hal_send_str("keccak_absorb/squeeze: ");
    hal_send_str(ullu(sum1 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / NTESTS));
    hal_send_str("\n");
    hal_send_str("RandomBytes/shake128: ");
    hal_send_str(ullu(sum3 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum4 / NTESTS));
    hal_send_str("\n");

    sum1 = sum2 = 0;
    for (i = 0; i < NTESTS; i++) {
        t1 = hal_get_time();
        sha3_256(buf, buf, 64);
        t2 = hal_get_time();
        sum1 += (t2 - t1);

        t1 = hal_get_time();
        sha3_512(buf, buf, 64);
        t2 = hal_get_time();
        sum2 += (t2 - t1);
    }
    hal_send_str("sha3_256/sha3_512: ");
    hal_send_str(ullu(sum1 / NTESTS));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / NTESTS));
    hal_send_str("\n");
    return 0;
}

static int SpeedNTT(void)
{
    uint64_t t1, t2, sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
    uint16_t a[SABER_N];
    uint32_t t[SABER_N], b[SABER_N];
    int k;
    for (k = 0; k < 1000; k++) {
        t1 = hal_get_time();
        NTT(a, t);
        t2 = hal_get_time();
        sum1 += (t2 - t1);

        t1 = hal_get_time();
        PolyBaseMul(t, b);
        t2 = hal_get_time();
        sum2 += (t2 - t1);

        t1 = hal_get_time();
        InvNTT(t, t);
        t2 = hal_get_time();
        sum3 += (t2 - t1);

        t1 = hal_get_time();
        PolyMulAcc((uint16_t*)a, (uint16_t*)t, (uint16_t*)b);
        t2 = hal_get_time();
        sum4 += (t2 - t1);
    }
    hal_send_str("NTT/BaseMul/INTT/PolyMulAcc is:");
    hal_send_str(ullu(sum1 / 1000));
    hal_send_str("/");
    hal_send_str(ullu(sum2 / 1000));
    hal_send_str("/");
    hal_send_str(ullu(sum3 / 1000));
    hal_send_str("/");
    hal_send_str(ullu(sum4 / 1000));
    hal_send_str("\n");
    return 0;
}

#endif

static void TestNTTRange(void)
{
    int i;
    uint16_t a[SABER_N * 2], r[SABER_N] = {0};
    uint16_t s[SABER_N];
    for (i = 0; i < SABER_N; i++) {
        s[i] = 5;
        a[i] = 4095;
    }
    PolyMulAcc(a, s, r);
    for (i = 0; i < SABER_N; i++) {
        printcycles("", r[i] & 0x1fff);
    }
}

static void PrintConfig(void)
{
    printcycles("SABER_L is %d\n", SABER_L);
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
    // SpeedCCAKP();
    // SpeedCCAEnc();
    // SpeedCCADec();
    return 0;
}
