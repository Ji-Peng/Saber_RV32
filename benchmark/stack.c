#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "poly.h"
#include "poly_mul.h"

// the address of a is 0x800035bf
// the top address of stack is 0x80004000
// -64 for not clear and check heap memory

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

// #define TEST_CCA
#ifdef TEST_CCA
// 0x20a0 is ok
// 992+1088+1440+32+32=3584
// 0x2800-3584=0x1a00
uint8_t pk[CRYPTO_PUBLICKEYBYTES];
uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
uint8_t sk[CRYPTO_SECRETKEYBYTES];
uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];
#    define MAX_SIZE (0x1a00 - 64)
unsigned int canary_size = MAX_SIZE;
// // uint8_t *pk = sk;
// // uint8_t *ct = sk;
static int test_stack(void)
{
    volatile unsigned char a;

    FILL_STACK()
    indcpa_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("indcpa_kem_keypair  %u\n", c);

    FILL_STACK()
    indcpa_kem_enc(ss_a, ss_b, pk, ct);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("indcpa_kem_enc      %u\n", c);

    FILL_STACK()
    indcpa_kem_dec(sk, ct, ss_b);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("indcpa_kem_dec      %u\n", c);

    FILL_STACK()
    crypto_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("crypto_kem_keypair  %u\n", c);

    FILL_STACK()
    crypto_kem_enc(ct, ss_a, pk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("crypto_kem_enc      %u\n", c);

    FILL_STACK()
    crypto_kem_dec(ss_b, ct, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("crypto_kem_dec      %u\n", c);

    if (memcmp(ss_a, ss_b, SABER_KEYBYTES)) {
        printf("KEM ERROR\n");
    }
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

    FILL_STACK()
    InnerProdInTime(pk, b, A);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("InnerProdInTime     %u\n", c);

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