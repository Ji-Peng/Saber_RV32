#include <stdint.h>
#include <stdio.h>

#define M 25166081
// M * Mprime = -1 mod R
#define Mprime 41877759
// R = MONT = 2^32
#define RmodM -8432555
// MONT^2/64
#define NINV 7689784

// NTT：normal bit reverse order
// br(1,2,3,...,63)
int32_t tree_ntt[] = {32, 16, 48, 8,  40, 24, 56, 4,  36, 20, 52, 12, 44,
                      28, 60, 2,  34, 18, 50, 10, 42, 26, 58, 6,  38, 22,
                      54, 14, 46, 30, 62, 1,  33, 17, 49, 9,  41, 25, 57,
                      5,  37, 21, 53, 13, 45, 29, 61, 3,  35, 19, 51, 11,
                      43, 27, 59, 7,  39, 23, 55, 15, 47, 31, 63};

int32_t tree_intt[] = {1,  33, 17, 49, 9,  41, 25, 57, 5,  37, 21, 53, 13,
                       45, 29, 61, 3,  35, 19, 51, 11, 43, 27, 59, 7,  39,
                       23, 55, 15, 47, 31, 63, 2,  34, 18, 50, 10, 42, 26,
                       58, 6,  38, 22, 54, 14, 46, 30, 62, 4,  36, 20, 52,
                       12, 44, 28, 60, 8,  40, 24, 56, 16, 48, 32};

// NTT：按照层合并时常数的出现顺序
int32_t tree_ntt_merged[] = {32, 16, 48, 8,  40, 24, 56, 4,  2,  34, 1,  33, 17,
                             49, 36, 18, 50, 9,  41, 25, 57, 20, 10, 42, 5,  37,
                             21, 53, 52, 26, 58, 13, 45, 29, 61, 12, 6,  38, 3,
                             35, 19, 51, 44, 22, 54, 11, 43, 27, 59, 28, 14, 46,
                             7,  39, 23, 55, 60, 30, 62, 15, 47, 31, 63};

// INTT: 按照层合并时常数出现的顺序
int32_t tree_intt_merged[] = {
    4,  2,  34, 1,  33, 17, 49, 36, 18, 50, 9,  41, 25, 57, 20, 10,
    42, 5,  37, 21, 53, 52, 26, 58, 13, 45, 29, 61, 12, 6,  38, 3,
    35, 19, 51, 44, 22, 54, 11, 43, 27, 59, 28, 14, 46, 7,  39, 23,
    55, 60, 30, 62, 15, 47, 31, 63, 32, 16, 48, 8,  40, 24, 56};

int32_t tree_mul_table[] = {
    1, 65, 33, 97,  17, 81, 49, 113, 9,  73, 41, 105, 25, 89, 57, 121,
    5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
    3, 67, 35, 99,  19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
    7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127};

const int root_table[64] = {
    2921358,   -10203707, -1203107,  6577444,   1776511,  -4194664, 5735629,
    7301157,   -4359117,  5669200,   -9600669,  10575964, 8064557,  -819256,
    -9084979,  -7944926,  1686897,   -588496,   -8693794, -7460755, 2723061,
    -11637995, -4810496,  7146164,   4092287,   -3261033, -5563113, -11307548,
    -7261676,  -4293923,  -6267356,  -9567042,  11980428, 6931502,  2510833,
    4034819,   -1988985,  -8060830,  -10319196, -6726360, 10171507, 8693725,
    647681,    -9344183,  2733537,   -42688,    10505644, -9502337, 10910265,
    2695651,   11450840,  -12030083, 5318976,   -1134236, -614272,  -6236460,
    -2559945,  -908786,   -2665284,  5184115,   -1069349, -9233574, 12174351};

const int inv_root_table[65] = {
    2559945,   2665284,  908786,    -12174351, 9233574,   1069349,   -5184115,
    -2695651,  12030083, -11450840, 6236460,   614272,    1134236,   -5318976,
    -647681,   -2733537, 9344183,   -10910265, 9502337,   -10505644, 42688,
    -4034819,  8060830,  1988985,   -8693725,  -10171507, 6726360,   10319196,
    7261676,   6267356,  4293923,   -2510833,  -6931502,  -11980428, 9567042,
    11637995,  -7146164, 4810496,   11307548,  5563113,   3261033,   -4092287,
    9084979,   -1686897, 7944926,   -2723061,  7460755,   8693794,   588496,
    -7301157,  -5669200, 4359117,   819256,    -8064557,  -10575964, 9600669,
    -11182464, 1203107,  10203707,  -5735629,  4194664,   -1776511,  -6577444,
    7689784,   RmodM};

const int mul_table[64] = {
    -9600669, 9600669,   10575964,  -10575964, 8064557,   -8064557,  -819256,
    819256,   -588496,   588496,    -8693794,  8693794,   -7460755,  7460755,
    2723061,  -2723061,  4092287,   -4092287,  -3261033,  3261033,   -5563113,
    5563113,  -11307548, 11307548,  -9567042,  9567042,   11980428,  -11980428,
    6931502,  -6931502,  2510833,   -2510833,  -10319196, 10319196,  -6726360,
    6726360,  10171507,  -10171507, 8693725,   -8693725,  -42688,    42688,
    10505644, -10505644, -9502337,  9502337,   10910265,  -10910265, 5318976,
    -5318976, -1134236,  1134236,   -614272,   614272,    -6236460,  6236460,
    5184115,  -5184115,  -1069349,  1069349,   -9233574,  9233574,   12174351,
    -12174351};

int32_t root[] = {9849271};

int32_t montgomery_reduce(int64_t a)
{
    int32_t t;

    t = (int32_t)a * Mprime;
    t = ((int64_t)a + (int64_t)t * M) >> 32;
    return t;
}

int32_t fqmul(int32_t a, int32_t b)
{
    return montgomery_reduce((int64_t)a * b);
}

int32_t my_pow(int32_t root, int32_t n)
{
    int32_t t = 1;
    for (int i = 0; i < n; i++) {
        t = fqmul(t, ((int64_t)root * RmodM) % M);
    }
    return t;
}

// 复现NTT Multiplication for NTT-unfriendly Rings论文中saber ntt的相关常数
void check(void)
{
    int32_t t;
    //   printf("%d\n", my_pow(3773600, 32));
    int my_root_table[63] = {};
    int my_inv_root_table[63] = {};

    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 63; j++) {
            t = my_pow(root[i], tree_ntt_merged[j]);
            t = fqmul(t, ((int64_t)RmodM * RmodM) % M);
            if (t > M / 2)
                t -= M;
            if (t < -M / 2)
                t += M;
            // printf("%d, ", t);
            my_root_table[j] = t;
            if (my_root_table[j] != root_table[j]) {
                printf("root_table error: %d\n", j);
            }
        }
        // printf("\n\n");
    }

    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 63; j++) {
            t = my_pow(root[i], 128 - tree_intt_merged[j]);
            t = fqmul(t, ((int64_t)RmodM * RmodM) % M);
            if (t > M / 2)
                t -= M;
            if (t < -M / 2)
                t += M;
            my_inv_root_table[j] = t;
            if (my_inv_root_table[j] != inv_root_table[j]) {
                printf("inv_root_table error: %d\n", j);
                printf("tree_inv[j] is %d\n", tree_intt_merged[j]);
                printf("my is %d, right is %d\n", my_inv_root_table[j],
                       inv_root_table[j]);
                printf("my*NINV is %d\n",
                       fqmul(my_inv_root_table[j], ((int64_t)NINV) % M));
            }
            // printf("%d, ", t);
        }
        // printf("\n\n");
    }

    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 64; j++) {
            t = my_pow(root[i], tree_mul_table[j]);
            t = fqmul(t, ((int64_t)RmodM * RmodM) % M);
            if (t > M / 2)
                t -= M;
            if (t < -M / 2)
                t += M;
            // printf("%d, ", t);
            if (t != mul_table[j]) {
                printf("mul_table[%d] error, my:%d, right:%d\n", j, t,
                       mul_table[j]);
            }
        }
        printf("check mul_table end\n");
    }
}

void generate_tables(void)
{
    int32_t t;
    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 63; j++) {
            t = my_pow(root[i], tree_ntt[j]);
            t = fqmul(t, ((int64_t)RmodM * RmodM) % M);
            // if (t > M / 2) t -= M;
            // if (t < -M / 2) t += M;
            printf("%d, ", t);
        }
    }
    printf("\n\n");

    for (int i = 0; i < 1; i++) {
        for (int j = 0; j < 63; j++) {
            t = my_pow(root[i], 128 - tree_intt[j]);
            t = fqmul(t, ((int64_t)RmodM * RmodM) % M);
            // if (t > M / 2) t -= M;
            // if (t < -M / 2) t += M;
            printf("%d, ", t);
        }
    }
}

void test_centered_method(void)
{
    uint16_t t1;
    for (int i = 0; i < 8192; i++) {
        t1 = i;
        t1 = ((int16_t)(t1 << 3)) >> 3;
        printf("%hd ", t1);
    }
}

void gen_constant(void)
{
    int32_t t, m = 4205569;
    t = M * Mprime;
    // printf("%d\n", t);
    for (int i = 1; i != 0; i++) {
        t = m * i;
        if (t == -1) {
            printf("Mprime is %d\n", i);
        }
        if (t == 1) {
            printf("MINV is %d\n", i);
        }
    }
}

void gen_constant1(void)
{
    int32_t t, m = 4205569;
    t = ((int64_t)1 << 32) % M;
    printf("%d\n", t - 25166081);
    t = ((int64_t)1 << 32) % m;
    printf("%d\n", t);
}

int main(void)
{
    // check();
    // generate_tables();
    // printf("%d\n",fqmul(32,NINV));
    // test_centered_method();
    // gen_constant();
    gen_constant1();
}