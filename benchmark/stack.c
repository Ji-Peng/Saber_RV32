#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SABER_params.h"
#include "api.h"
#include "poly_mul.h"

// the address of a is 0x800035bf
// the top address of stack is 0x80004000
// -64 for not clear and check heap memory
#define MAX_SIZE (0x1a40 - 64)

unsigned int canary_size = MAX_SIZE;
volatile unsigned char *p;
unsigned int c;
uint8_t canary = 0x42;

// reuse sk for saving memory
uint8_t sk[CRYPTO_SECRETKEYBYTES];
uint8_t *pk = sk;
uint8_t *ct = sk;
uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];
unsigned int stack_keygen, stack_encaps, stack_decaps;

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

static int test_stack(void)
{
    volatile unsigned char a;
    printf("the address of a is 0x%x\n", &a);
    // Alice generates a public key
    FILL_STACK()
    crypto_kem_keypair(pk, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    stack_keygen = c;

    // Bob derives a secret key and creates a response
    FILL_STACK()
    crypto_kem_enc(ct, ss_a, pk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    stack_encaps = c;

    // Alice uses Bobs response to get her secret key
    FILL_STACK()
    crypto_kem_dec(ss_b, ct, sk);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    stack_decaps = c;
    if (memcmp(ss_a, ss_b, CRYPTO_BYTES)) {
        printf("KEM ERROR\n");
    }
    printf("key gen stack usage %u\n", stack_keygen);
    printf("encaps stack usage %u\n", stack_encaps);
    printf("decaps stack usage %u\n", stack_decaps);
    return 0;
}

// uint16_t A[SABER_N], B[SABER_N], C[SABER_N];

// static int test_polmul_stack(void)
// {
//     volatile unsigned char a;
//     // Alice generates a public key
//     FILL_STACK()
//     pol_mul(A, B, C);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("pol_mul stack usage %u\n", c);
//     return 0;
// }

// uint8_t seed[SABER_SEEDBYTES];
// uint16_t skpv[SABER_K][SABER_N];
// uint8_t ct[CRYPTO_CIPHERTEXTBYTES];

// static int test_matrixvector_stack(void)
// {
//     volatile unsigned char a;
//     FILL_STACK()
//     MatrixVectorMul_keypair(seed, skpv, skpv, SABER_Q - 1);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("MatrixVectorMul_keypair stack usage %u\n", c);

//     FILL_STACK()
//     MatrixVectorMul_encryption(seed, skpv, ct, SABER_Q - 1);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("MatrixVectorMul_encryption stack usage %u\n", c);

//     FILL_STACK()
//     VectorMul(ct, skpv, skpv[SABER_K - 1]);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("VectorMul stack usage %u\n", c);
//     return 0;
// }

// uint16_t temp_ar[SABER_N];
// uint16_t r[SABER_K][SABER_N];
// uint8_t seed[SABER_SEEDBYTES];

// static int test_Genpoly_stack(void)
// {
//     volatile unsigned char a;
//     FILL_STACK()
//     GenMatrix_poly(temp_ar, seed, 0);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("GenMatrix_poly stack usage %u\n", c);

//     FILL_STACK()
//     GenSecret(r, seed);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     printf("GenSecret stack usage %u\n", c);
// }

int main(void)
{
    canary_size = MAX_SIZE;
    printf("==========stack test==========\n");
    test_stack();
    // test_Genpoly_stack();
    return 0;
}