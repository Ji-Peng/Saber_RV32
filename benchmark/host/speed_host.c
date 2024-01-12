#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "api.h"
#include "cpucycles_host.h"
#include "fips202.h"
#include "ntt.h"
#include "poly.h"
#include "poly_mul.h"
#include "reduce.h"
#include "rng.h"

static int TestNTT(void);
static int test_ntt2(void);
void test_poly_mul(void);

#define NTESTS 10000

uint64_t t[NTESTS];

static int TestNTT(void)
{
    uint16_t A[SABER_L][SABER_L][SABER_N];
    uint16_t s[SABER_L][SABER_N];
    uint16_t b[SABER_L][SABER_N] = {0};
    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    int i;
    unsigned char entropy_input[48];

    for (i = 0; i < 48; i++)
        entropy_input[i] = i + 1;
    RandomBytesInit(entropy_input, NULL);

    RandomBytes(seed_A, SABER_SEEDBYTES);
    shake128(seed_A, SABER_SEEDBYTES, seed_A,
             SABER_SEEDBYTES);  // for not revealing system RNG state
    RandomBytes(seed_s, SABER_NOISE_SEEDBYTES);

    GenSecret(s, seed_s);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        MatrixVectorMul_ntt((int16_t(*)[3][256])A, (int16_t(*)[256])s,
                            (int16_t(*)[256])b, 1);
    }
    print_results("MatrixVectorMul_ntt: ", t, NTESTS);

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        MatrixVectorMul(A, s, b, 1);
    }
    print_results("MatrixVectorMul: ", t, NTESTS);
    return 0;
}

static int test_ntt2(void)
{
    int16_t in[256];
    int32_t out1[256];
    int32_t i;

    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        NTT(in, out1);
    }
    print_results("NTT: ", t, NTESTS);

    return 0;
}

void test_poly_mul(void)
{
    int16_t a[SABER_N];
    int16_t b[SABER_N];
    int16_t res[SABER_N];
    uint64_t t1, t2;
    t1 = cpucycles();
    PolyMulAcc((uint16_t *)a, (uint16_t *)b, (uint16_t *)res);
    t2 = cpucycles();
    printf("PolyMulAcc cycles is %lu\n", (t2 - t1));
    t1 = cpucycles();
    PolyMulAcc(a, b, res);
    t2 = cpucycles();
    printf("PolyMulAcc cycles is %lu\n", (t2 - t1));
}

/**
 * 基本思路：
 * part1: 是很简单的案例，基本模式是rdc(x*mont)=x mod q
 * part2: 是对输入边界的一些值作测试
 * part3：对不允许的输入进行测试，验证即可发现其输出范围超过了算法设计所给出的范围
 */
static void test_mont_rdc()
{
    // 2^32 mod q
    const int32_t mont = 5453415;
    int64_t c;

    printf("Module for Saber's NTT is %d\n\n", M);
    // part1
    c = 0 * mont;
    printf("MontReduce(%d*mont)=%d\n\n", 0, MontReduce(c));

    c = 1 * mont;
    printf("MontReduce(%d*mont)=%d\n\n", 1, MontReduce(c));

    c = -1 * mont;
    printf("MontReduce(%d*mont)=%d\n\n", -1, MontReduce(c));

    c = (int64_t)M / 2 * mont;
    printf("MontReduce(%d*mont)=%d\n\n", M / 2, MontReduce(c));

    c = -(int64_t)M / 2 * mont;
    printf("MontReduce(%d*mont)=%d\n\n", -M / 2, MontReduce(c));

    c = (int64_t)M * mont;
    printf("MontReduce(%d*mont)=%d\n\n", M, MontReduce(c));

    c = -(int64_t)M * mont;
    printf("MontReduce(%d*mont)=%d\n\n", -M, MontReduce(c));

    /**
     * part2
     * 输入所允许的范围为：{-q2^15,...,q2^15-1}。因此对边界值进行测试：-q2^15,
     * -q2^15+1, q2^15-2, q2^15-1
     */
    c = -(int64_t)M * (1 << 31);
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = -(int64_t)M * (1 << 31) + 1;
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = (int64_t)M * (1 << 31) - 2;
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = (int64_t)M * (1 << 31) - 1;
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    /**
     * part3
     * 反面案例，尝试对输入范围外的数进行约减
     */
    c = -((int64_t)M * 2 + 1) * (1 << 31) + 1;
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = -((int64_t)M * 3 + 1) * (1 << 31);
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = ((int64_t)M * 2 + 1) * (1 << 31);
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));

    c = ((int64_t)M * 3 + 1) * (1 << 31) + 1;
    printf("MontReduce(%ld)=%d\n\n", c, MontReduce(c));
}

static void print_poly_16b(char *s, int16_t *a)
{
    int i;
    printf("%s: ", s);
    for (i = 0; i < 255; i++) {
        printf("%d, ", a[i]);
    }
    printf("%d", a[i]);
    printf("\n");
}

static void print_poly(char *s, int32_t *a)
{
    int i;
    printf("%s: ", s);
    for (i = 0; i < 255; i++) {
        printf("%d, ", a[i]);
    }
    printf("%d", a[i]);
    printf("\n");
}

static void print_poly_32b_16b(char *s, int32_t *a)
{
    int i;
    printf("%s: ", s);
    for (i = 0; i < 255; i++) {
        printf("%d, ", (int16_t)a[i]);
    }
    printf("%d", (int16_t)a[i]);
    printf("\n");
}

#define SABER_Q (1 << 13)
static void test_ntt()
{
    int16_t a[256], b[256], c[256];
    int32_t a_32b[256], b_32b[256], c_32b[256];
    int i;

    // 简单例子
    for (i = 0; i < 256; i++) {
        a[i] = b[i] = 0;
    }
    a[0] = 1;
    b[0] = 1;
    print_poly_16b("a", a);
    print_poly_16b("b", b);
    NTT(a, a_32b);
    NTT(b, b_32b);
    print_poly("ntt(a)", a_32b);
    print_poly("ntt(b)", b_32b);
    PolyBaseMul(a_32b, b_32b);
    print_poly("after basemul", a_32b);
    InvNTT(a_32b, a_32b);
    print_poly("after invntt", a_32b);
    printf("\n");

    // 也是简单例子，a[0]=b[0]=q-1
    for (i = 0; i < 256; i++) {
        a[i] = b[i] = 0;
    }
    a[0] = SABER_Q - 1;
    b[0] = 3;
    print_poly_16b("a", a);
    print_poly_16b("b", b);
    NTT(a, a_32b);
    NTT(b, b_32b);
    print_poly("ntt(a)", a_32b);
    print_poly("ntt(b)", b_32b);
    PolyBaseMul(a_32b, b_32b);
    print_poly("after basemul", a_32b);
    InvNTT(a_32b, a_32b);
    print_poly("after invntt", a_32b);
    printf("\n");

    // 系数全为0
    for (i = 0; i < 256; i++) {
        a[i] = b[i] = 0;
    }
    print_poly_16b("a", a);
    print_poly_16b("b", b);
    NTT(a, a_32b);
    NTT(b, b_32b);
    print_poly("ntt(a)", a_32b);
    print_poly("ntt(b)", b_32b);
    PolyBaseMul(a_32b, b_32b);
    print_poly("after basemul", a_32b);
    InvNTT(a_32b, a_32b);
    print_poly("after invntt", a_32b);
    printf("\n");

    // 系数全为1
    for (i = 0; i < 256; i++) {
        a[i] = b[i] = 1;
    }
    print_poly_16b("a", a);
    print_poly_16b("b", b);
    NTT(a, a_32b);
    NTT(b, b_32b);
    print_poly("ntt(a)", a_32b);
    print_poly("ntt(b)", b_32b);
    PolyBaseMul(a_32b, b_32b);
    print_poly("after basemul", a_32b);
    InvNTT(a_32b, a_32b);
    print_poly("after invntt", a_32b);
    printf("\n");

    for (i = 0; i < 256; i++) {
        a[i] = SABER_Q - 1;
    }
    for (i = 0; i < 256; i++) {
        b[i] = 1;
    }
    print_poly_16b("a", a);
    print_poly_16b("b", b);
    NTT(a, a_32b);
    NTT(b, b_32b);
    print_poly("ntt(a)", a_32b);
    print_poly("ntt(b)", b_32b);
    PolyBaseMul(a_32b, b_32b);
    print_poly("after basemul", a_32b);
    InvNTT(a_32b, a_32b);
    print_poly("after invntt", a_32b);
    printf("\n");
}

static void print_bytes(char *s, uint8_t *k, int keylen)
{
    int i;

    printf("%s: ", s);
    for (i = 0; i < keylen - 1; i++) {
        printf("%u, ", k[i]);
    }
    printf("%u\n\n", k[i]);
}

static void test_kem()
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t key_a[CRYPTO_BYTES];
    uint8_t key_b[CRYPTO_BYTES];

    // Alice generates a public key
    crypto_kem_keypair(pk, sk);
    print_bytes("pk", pk, CRYPTO_PUBLICKEYBYTES);
    print_bytes("sk", sk, CRYPTO_SECRETKEYBYTES);

    // Bob derives a secret key and creates a response
    crypto_kem_enc(ct, key_b, pk);
    print_bytes("ct", ct, CRYPTO_CIPHERTEXTBYTES);
    print_bytes("key_b", key_b, CRYPTO_BYTES);

    // Alice uses Bobs response to get her shared key
    crypto_kem_dec(key_a, ct, sk);
    print_bytes("key_a", key_a, CRYPTO_BYTES);
}

int main(void)
{
    // TestNTT();
    // test_ntt2();
    // test_poly_mul();
    test_mont_rdc();
    test_ntt();
    test_kem();
    test_kem();
}
