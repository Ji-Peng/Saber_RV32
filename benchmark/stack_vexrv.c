#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "poly.h"
#include "poly_mul.h"
#include "hal.h"

volatile unsigned char *p;
unsigned int c;
uint8_t canary = 0x42;

#define FILL_STACK()               \
    p = &a;                        \
    while (p > (&a - canary_size)) \
        *(p--) = canary;

#define CHECK_STACK()                    \
    c = canary_size;                     \
    p = &a - canary_size + 1;            \
    while ((*p == canary) && (p < &a)) { \
        p++;                             \
        c--;                             \
    }

#define TEST_CCA
#ifdef TEST_CCA
// 992+1088+1440+32+32=3584
// 0x2800-3584=0x1a00
// uint8_t pk[CRYPTO_PUBLICKEYBYTES];
// uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
uint8_t sk[CRYPTO_SECRETKEYBYTES];
uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];
// -128 for avoiding affecting heap memory
#    define MAX_SIZE (0x18200 - 128)
unsigned int canary_size = MAX_SIZE;
uint8_t *pk = sk;
uint8_t *ct = sk;

static void printcycles(const char* s, unsigned long long c)
{
    char outs[32];
    hal_send_str(s);
    snprintf(outs, sizeof(outs), "%llu ", c);
    hal_send_str(outs);
}

static int test_stack(void)
{
    volatile unsigned char a;

    hal_send_str("indcpa_kem_keypair/enc/dec:");
    FILL_STACK()
    indcpa_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    FILL_STACK()
    indcpa_kem_enc(ss_a, ss_b, pk, ct);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    FILL_STACK()
    indcpa_kem_dec(sk, ct, ss_b);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    hal_send_str("\n crypto_kem_keypair/enc/dec:");
    FILL_STACK()
    crypto_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    FILL_STACK()
    crypto_kem_enc(ct, ss_a, pk);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    FILL_STACK()
    crypto_kem_dec(ss_b, ct, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        hal_send_str("c >= canary_size\n");
        return -1;
    }
    printcycles("", c);

    return 0;
}

#else
// 0x1800 is ok
uint8_t seed_A[SABER_SEEDBYTES];
uint8_t seed_s[SABER_NOISE_SEEDBYTES];
// uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES];
// uint8_t sk[SABER_INDCPA_SECRETKEYBYTES];
uint8_t ciphertext[SABER_BYTES_CCA_DEC];
uint8_t *pk = ciphertext;
uint8_t *sk = ciphertext;
// uint16_t A[2 * SABER_N];
// uint16_t C[SABER_N];
uint16_t b[SABER_L][SABER_N];
uint16_t *A = b;
uint16_t *C = b;
#    define MAX_SIZE (0x1800 - 64)
unsigned int canary_size = MAX_SIZE;
static int test_stack(void)
{
    volatile unsigned char a;
    FILL_STACK()
    MatrixVectorMulKP(seed_A, seed_s, sk, b);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("MatrixVectorMulKP   %u\n", c);

    FILL_STACK()
    MatrixVectorMulEnc(seed_A, b, ciphertext);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("MatrixVectorMulEnc  %u\n", c);

    // FILL_STACK()
    // InnerProdInTime(pk, b, A);
    // CHECK_STACK()
    // if (c >= canary_size) {
    //     printf("c >= canary_size\n");
    //     return -1;
    // }
    // printf("InnerProdInTime     %u\n", c);

    FILL_STACK()
    PolyMulAcc(A, (uint16_t *)b, C);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("PolyMulAcc          %u\n", c);

    FILL_STACK()
    GenAInTime(A, seed_A, 0);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("GenAInTime          %u\n", c);

    FILL_STACK()
    GenSInTime(A, seed_s, 0);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("GenSInTime     %u\n", c);
    return 0;
}
#endif

static void PrintConfig(void)
{
    printcycles("SABER_L is: ", SABER_L);
//     hal_send_str("Strategy: ");
// #    ifdef FASTGENA_SLOWMUL
//     hal_send_str("FASTGENA_SLOWMUL ");
// #    elif defined(FASTGENA_FASTMUL)
//     hal_send_str("FASTGENA_FASTMUL ");
// #    elif defined(SLOWGENA_FASTMUL)
//     hal_send_str("SLOWGENA_FASTMUL ");
// #    endif

//     hal_send_str("NTT: ");
// #    ifdef COMPLETE_NTT
//     hal_send_str("COMPLETE_NTT ");
// #    elif defined(SEVEN_LAYER_NTT)
//     hal_send_str("SEVEN_LAYER_NTT ");
// #    elif defined(SIX_LAYER_NTT)
//     hal_send_str("SIX_LAYER_NTT ");
// #    elif defined(FIVE_LAYER_NTT)
//     hal_send_str("FIVE_LAYER_NTT ");
// #    endif

// #    ifdef NTTASM
//     hal_send_str("ASM Implementation\n");
// #    else
//     hal_send_str("C Implementation\n");
// #    endif
}

int main(void)
{
    canary_size = MAX_SIZE;
    // printf("==========stack test==========\n");
	hal_setup(CLOCK_BENCHMARK);
	PrintConfig();
    test_stack();
    return 0;
}