#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "poly.h"
#include "poly_mul.h"

#ifdef PQRISCV_PLATFORM
#    include "hal.h"
#    define printf hal_send_str
#endif

char outs[32];
volatile unsigned char *p;
unsigned int c;
uint8_t canary = 0x42;

#define PRINTCYCLES()                       \
    snprintf(outs, sizeof(outs), "%u ", c); \
    printf(outs);                     \
    printf("\n");

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
// 0x1a00 for sifive, 0x14000 for vexriscv
#    define MAX_SIZE (0x1a00 - 128)
unsigned int canary_size = MAX_SIZE;
uint8_t *pk = sk;
uint8_t *ct = sk;
static int test_stack(void)
{
    volatile unsigned char a;

    printf("crypto_kem_keypair,enc,dec:\n");
    FILL_STACK()
    crypto_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    PRINTCYCLES()

    FILL_STACK()
    crypto_kem_enc(ct, ss_a, pk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    PRINTCYCLES()

    FILL_STACK()
    crypto_kem_dec(ss_b, ct, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    PRINTCYCLES()

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

int main(void)
{
    canary_size = MAX_SIZE;
    printf("==========stack test==========\n");
    test_stack();
    return 0;
}