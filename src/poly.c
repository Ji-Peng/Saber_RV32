#include "poly.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "cbd.h"
#include "fips202.h"
#include "ntt.h"
#include "pack_unpack.h"
#include "poly_mul.h"

void GenSecret(uint8_t s[SABER_L][SABER_N],
               const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t buf[SABER_L * SABER_POLYCOINBYTES];
    int32_t i;

    shake128(buf, sizeof(buf), seed, SABER_NOISE_SEEDBYTES);

    for (i = 0; i < SABER_L; i++) {
        CBD(s[i], buf + i * SABER_POLYCOINBYTES, SABER_N);
    }
}

void GenSInTime(uint8_t s[SABER_N], const uint8_t seed[SABER_NOISE_SEEDBYTES],
                int32_t index)
{
    int32_t i;
    // keccak states
    static uint64_t keccak_state[25];

    if (index == 0) {
        // init
        for (i = 0; i < 25; i++)
            keccak_state[i] = 0;
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, SABER_NOISE_SEEDBYTES,
                      0x1F);
    }

#if SABER_MU == 6
    uint8_t buf[SHAKE128_RATE];
    static uint8_t leftovers[72];
    if (index == 0) {
        // 1buf = 224coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s, buf, 224);
        // 1buf = 32coeff + 72B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 224, buf, 32);
        memcpy(leftovers, buf + SHAKE128_RATE - sizeof(leftovers),
               sizeof(leftovers));
    } else if (index == 1) {
        // 1leftover = 72B = 96coeff
        CBD(s, leftovers, 96);
        // 1buf = 160coeff + 48B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 96, buf, 160);
        memcpy(leftovers, buf + SHAKE128_RATE - 48, 48);
    } else if (index == 2) {
        // 48B leftover = 64coeff
        CBD(s, leftovers, 64);
        // 1buf = 192coeff + 24B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 64, buf, 192);
        memcpy(leftovers, buf + SHAKE128_RATE - 24, 24);
    } else if (index == 3) {
        // 24B leftover = 32coeff
        CBD(s, leftovers, 32);
        // 1buf = 224coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 32, buf, 224);
    } else {
        printf("Error in GenSInTime\n");
        exit(1);
    }

#elif SABER_MU == 8
    uint8_t buf[SHAKE128_RATE];
    static uint8_t leftovers[88];
    if (index == 0) {
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s, buf, 168);
        // 1buf = 88coeff + 80B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 168, buf, 88);
        memcpy(leftovers, buf + SHAKE128_RATE - 80, 80);
    } else if (index == 1) {
        // 1leftover = 80B = 80coeff
        CBD(s, leftovers, 80);
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 80, buf, 168);
        // 1buf = 8coeff + 88B leftover
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 248, buf, 8);
        memcpy(leftovers, buf + SHAKE128_RATE - 88, 88);
    } else if (index == 2) {
        // 1leftover = 88B = 88coeff
        CBD(s, leftovers, 88);
        // 1buf = 168coeff
        keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
        CBD(s + 88, buf, 168);
    } else {
        printf("Error in GenSInTime\n");
        exit(1);
    }

#elif SABER_MU == 10
    uint8_t buf[SHAKE128_RATE];
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    CBD(s, buf, SABER_N / 2);
    keccak_squeezeblocks(buf, 1, keccak_state, SHAKE128_RATE);
    CBD(s + SABER_N / 2, buf, SABER_N / 2);
#else
#    error "Unsupported SABER parameter."
#endif
}

void GenSecretNTT(int32_t s[SABER_L][SABER_N],
                  const uint8_t seed[SABER_NOISE_SEEDBYTES])
{
    uint8_t t[SABER_N];
    int i;

    for (i = 0; i < SABER_L; i++) {
        GenSInTime(t, seed, i);
        NTTS(t, s[i]);
    }
}

void CenteredReduce(uint16_t poly[SABER_N])
{
    int32_t i;
    for (i = 0; i < SABER_N; i++) {
        poly[i] = (int16_t)(poly[i] << 3) >> 3;
    }
}

#if defined(FASTGENA_SLOWMUL) || defined(FASTGENA_FASTMUL)
/**
 * @description: Generate polynomial on the fly
 */
void GenAInTime(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
                uint32_t init)
{
    int32_t i;
    // 416B = 1poly, 3*168-416=88
    uint8_t buf[3 * SHAKE128_RATE];
    // 80+168*2=416B=1poly, so save 80B leftovers is enough
    static uint8_t leftovers[80];
    // keccak states
    static uint64_t keccak_state[25];
    // 0: squeeze 3blocks and generate 1 poly;
    // 1: squeeze 2blocks and recover leftovers && gen 1 poly;
    static int32_t state;
    // init: clear states and absorb seed
    if (init == 1) {
        for (i = 0; i < 25; i++)
            keccak_state[i] = 0;
        keccak_absorb(keccak_state, SHAKE128_RATE, seed, SABER_SEEDBYTES, 0x1F);
        state = 0;
    }
    if (state == 0) {
        // squeeze 3blocks
        keccak_squeezeblocks(buf, 3, keccak_state, SHAKE128_RATE);
        // 416B = 1poly
        BS2Polq(buf, poly);
        // save leftovers
        memcpy(leftovers, buf + sizeof(buf) - sizeof(leftovers),
               sizeof(leftovers));
    } else {
        // squeeze 2blocks
        keccak_squeezeblocks(buf, 2, keccak_state, SHAKE128_RATE);
        // get leftovers
        memcpy(buf + 2 * SHAKE128_RATE, leftovers, sizeof(leftovers));
        // generate 1poly
        BS2Polq(buf, poly);
    }
    state = !state;
    CenteredReduce(poly);
}
#elif defined(SLOWGENA_FASTMUL)
/**
 * @description: Generate polynomial matrix A on the fly
 */
void GenAInTime(uint16_t poly[SABER_N], const uint8_t seed[SABER_SEEDBYTES],
                int32_t x, int32_t y)
{
    int i;
    uint64_t states[25];
    uint8_t buf[3 * SHAKE128_RATE];
    // extended seed
    uint8_t xseed[SABER_SEEDBYTES + 2];
    for (i = 0; i < 25; i++) {
        states[i] = 0;
    }
    memcpy(xseed, seed, SABER_SEEDBYTES);
    xseed[SABER_SEEDBYTES + 0] = x;
    xseed[SABER_SEEDBYTES + 1] = y;
    keccak_absorb(states, SHAKE128_RATE, xseed, SABER_SEEDBYTES + 2, 0x1F);
    keccak_squeezeblocks(buf, 3, states, SHAKE128_RATE);
    BS2Polq(buf, poly);
    CenteredReduce(poly);
}

#endif