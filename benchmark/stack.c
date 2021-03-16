#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "poly.h"
#include "poly_mul.h"

// the address of a is 0x800035bf
// the top address of stack is 0x80004000
// -64 for not clear and check heap memory
#define MAX_SIZE (0x1600 - 64)

unsigned int canary_size = MAX_SIZE;
volatile unsigned char *p;
unsigned int c;
uint8_t canary = 0x42;

// in order to save memory, reuse sk
// uint8_t sk[CRYPTO_SECRETKEYBYTES];
// uint8_t *pk = sk;
// uint8_t *ct = sk;
// uint8_t ss_a[CRYPTO_BYTES], ss_b[CRYPTO_BYTES];
// unsigned int stack_keygen, stack_encaps, stack_decaps;

// test MatrixVectorMul and InnerProd
uint16_t A[SABER_L][SABER_L][SABER_N];

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

// static int test_stack(void)
// {
//     volatile unsigned char a;
//     // Alice generates a public key
//     FILL_STACK()
//     crypto_kem_keypair(pk, sk);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     stack_keygen = c;

//     // Bob derives a secret key and creates a response
//     FILL_STACK()
//     crypto_kem_enc(ct, ss_a, pk);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     stack_encaps = c;

//     // Alice uses Bobs response to get her secret key
//     FILL_STACK()
//     crypto_kem_dec(ss_b, ct, sk);
//     CHECK_STACK()
//     if (c >= canary_size) {
//         printf("c >= canary_size\n");
//         return -1;
//     }
//     stack_decaps = c;
//     if (memcmp(ss_a, ss_b, CRYPTO_BYTES)) {
//         printf("KEM ERROR\n");
//     }
//     printf("key gen stack usage %u\n", stack_keygen);
//     printf("encaps stack usage %u\n", stack_encaps);
//     printf("decaps stack usage %u\n", stack_decaps);
//     return 0;
// }

static int test_polmul_stack(void)
{
    volatile unsigned char a;
    FILL_STACK()
    MatrixVectorMul(A, A, A, 1);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("MatrixVectorMul stack usage %u\n", c);

    FILL_STACK()
    InnerProd(A, A, A);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("InnerProd stack usage %u\n", c);

    FILL_STACK()
    poly_mul_acc(A, A, A);
    CHECK_STACK()
    if (c >= canary_size) {
        printf("c >= canary_size\n");
        return -1;
    }
    printf("poly_mul_acc stack usage %u\n", c);
    return 0;
}

int main(void)
{
    canary_size = MAX_SIZE;
    printf("==========stack test==========\n");
    test_polmul_stack();
    return 0;
}