#include "reduce.h"

#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        MontReduce
 *
 * Description: Montgomery reduction; given a 64-bit integer a, computes
 *              32-bit integer congruent to a * R^-1 mod M, where R=2^32
 *
 * Arguments:   - int64_t a: input integer to be reduced; input should be in
 * {-M2^31, ..., M2^31-1},otherwise the output range cannot be guaranteed.
 *
 * Returns:     integer in {-M+1,...,M-1} congruent to a * R^-1 modulo M.
 **************************************************/
int32_t MontReduce(int64_t a)
{
    int32_t t;

    t = (int32_t)a * Mprime;
    t = ((int64_t)a + (int64_t)t * M) >> 32;
    return t;
}

/*************************************************
 * Name:        BarrettReduce
 *
 * Description: Barrett reduction; given a 32-bit integer a, computes
 * centered representative congruent to a mod M in
 * {-(M-1)/2,...,(M-1)/2}
 *
 * Arguments:   - int32_t a: input integer to be reduced
 *
 * Returns:     integer in {-(M-1)/2,...,(M-1)/2} congruent to a modulo M.
 **************************************************/
int32_t BarrettReduce(int32_t a)
{
    int32_t t;
    const int32_t v = (((int64_t)1 << 48) + M / 2) / M;

    t = ((int64_t)v * a + ((int64_t)1 << 47)) >> 48;
    t *= M;
    return a - t;
}

// int main(void)
// {
//     printf("%d\n", MontReduce(((int64_t)409 * M + 3) * RmodM));
//     printf("%d\n", MontReduce((int64_t)(M - 1) * RmodM));
//     printf("%d\n", MontReduce((int64_t)(2 * M) * RmodM));
//     printf("%d\n", MontReduce((int64_t)(2 * M - 1) * RmodM));
//     printf("%d\n", MontReduce((int64_t)(3 * M) * RmodM));
//     printf("%d\n", MontReduce((int64_t)(3 * M - 1) * RmodM));
//     printf("%d\n", BarrettReduce(409 * M - 3));
//     printf("%d\n", BarrettReduce(M - 1));
//     printf("%d\n", BarrettReduce(2 * M));
//     printf("%d\n", BarrettReduce(2 * M - 1));
//     printf("%d\n", BarrettReduce(M / 2 + 3));
// }