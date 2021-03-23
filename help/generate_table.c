#include <stdint.h>
#include <stdio.h>

#define M 10487809      // M = 512 * 20484 + 1 < 2^24
#define Mprime 6293503  // M * Mprime = -1 mod R (R=2^32)
#define MINV -6293503   // M * MINV = 1 mod R
#define RmodM 5453415   // R mod M
#define NINV 2466627    // R^2 * (1/64) mod M

// 512th root is 23394
// 128th root = 23394^4
int32_t root = 8406460;
int64_t R = (int64_t)1 << 32;

// NTT：normal bit reverse order
// br(1,2,3,...,63)
int32_t treeNTT[] = {32, 16, 48, 8,  40, 24, 56, 4,  36, 20, 52, 12, 44,
                     28, 60, 2,  34, 18, 50, 10, 42, 26, 58, 6,  38, 22,
                     54, 14, 46, 30, 62, 1,  33, 17, 49, 9,  41, 25, 57,
                     5,  37, 21, 53, 13, 45, 29, 61, 3,  35, 19, 51, 11,
                     43, 27, 59, 7,  39, 23, 55, 15, 47, 31, 63};

int32_t treeINTT[] = {1,  33, 17, 49, 9,  41, 25, 57, 5,  37, 21, 53, 13,
                      45, 29, 61, 3,  35, 19, 51, 11, 43, 27, 59, 7,  39,
                      23, 55, 15, 47, 31, 63, 2,  34, 18, 50, 10, 42, 26,
                      58, 6,  38, 22, 54, 14, 46, 30, 62, 4,  36, 20, 52,
                      12, 44, 28, 60, 8,  40, 24, 56, 16, 48, 32};

int32_t treeNTTMerged[] = {32, 16, 48, 8,  40, 24, 56, 4,  2,  34, 1,  33, 17,
                           49, 36, 18, 50, 9,  41, 25, 57, 20, 10, 42, 5,  37,
                           21, 53, 52, 26, 58, 13, 45, 29, 61, 12, 6,  38, 3,
                           35, 19, 51, 44, 22, 54, 11, 43, 27, 59, 28, 14, 46,
                           7,  39, 23, 55, 60, 30, 62, 15, 47, 31, 63};

int32_t treeINTTMerged[] = {4,  2,  34, 1,  33, 17, 49, 36, 18, 50, 9,  41, 25,
                            57, 20, 10, 42, 5,  37, 21, 53, 52, 26, 58, 13, 45,
                            29, 61, 12, 6,  38, 3,  35, 19, 51, 44, 22, 54, 11,
                            43, 27, 59, 28, 14, 46, 7,  39, 23, 55, 60, 30, 62,
                            15, 47, 31, 63, 32, 16, 48, 8,  40, 24, 56};

int32_t treeMulTable[] = {
    1, 65, 33, 97,  17, 81, 49, 113, 9,  73, 41, 105, 25, 89, 57, 121,
    5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
    3, 67, 35, 99,  19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
    7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127};

int32_t MontReduce(int64_t a)
{
    int32_t t;

    t = (int32_t)a * Mprime;
    t = ((int64_t)a + (int64_t)t * M) >> 32;
    return t;
}

int32_t FqMul(int32_t a, int32_t b)
{
    return MontReduce((int64_t)a * b);
}

int32_t Pow(int32_t root, int32_t n)
{
    int32_t t = 1;
    for (int i = 0; i < n; i++) {
        t = FqMul(t, ((int64_t)root * RmodM) % M);
    }
    return t;
}

void check(void)
{
    // check 128th root
    if (Pow(root, 128) == 1) {
        printf("check 128th root    passed\n");
    } else {
        printf("check 128th root    error\n");
    }
    // check MPrime
    int32_t m = M, mp = Mprime;
    if ((int64_t)m * mp % R == -1 || (int64_t)m * mp % R - R == -1) {
        printf("check MPrime        passed\n");
    } else {
        printf("check MPrime        error\n");
    }
    // check MINV
    mp = MINV;
    if ((int64_t)m * mp % R == 1 || (int64_t)m * mp % R + R == 1) {
        printf("check MINV          passed\n");
    } else {
        printf("check MINV          error\n");
    }
    // check RmodM
    if (R % M == RmodM) {
        printf("check RmodM         passed\n");
    } else {
        printf("check RmodM         error\n");
    }
    // check NINV
    if ((R >> 6) * R % M == NINV) {
        printf("check NINV          passed\n");
    } else {
        printf("check NINV          error\n");
    }
    printf("===============check end===============\n");
}

void GenTables(void)
{
    int32_t t;
    for (int j = 0; j < 63; j++) {
        t = Pow(root, treeNTT[j]);
        t = FqMul(t, ((int64_t)RmodM * RmodM) % M);
        printf("%d, ", t);
    }
    printf("\n");

    for (int j = 0; j < 63; j++) {
        // root is 128th, -i in intt
        t = Pow(root, 128 - treeINTT[j]);
        t = FqMul(t, ((int64_t)RmodM * RmodM) % M);
        printf("%d, ", t);
    }
    printf("\n");

    for (int j = 0; j < 64; j++) {
        t = Pow(root, treeMulTable[j]);
        t = FqMul(t, ((int64_t)RmodM * RmodM) % M);
        printf("%d, ", t);
    }
    printf("\n");
}

int main(void)
{
    check();
    GenTables();
    return 0;
}