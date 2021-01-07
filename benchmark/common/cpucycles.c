#include "cpucycles.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

char* ullu(uint64_t val)
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

    // *out-- = 'x', *out = '0';

    return ++out;
}

// int main(void)
// {
//     uint64_t hval = 0xffffffffffffffff;
//     printf("%s\n", ullu(hval));
//     return 0;
// }