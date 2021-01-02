#include "SABER_indcpa.h"

#include <stdint.h>
#include <string.h>

#include "SABER_params.h"
#include "fips202.h"
#include "pack_unpack.h"
#include "poly.h"
#include "poly_mul.h"
#include "rng.h"

void indcpa_kem_keypair(uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES],
                        uint8_t sk[SABER_INDCPA_SECRETKEYBYTES])
{
    uint16_t s[SABER_L][SABER_N];
    uint16_t b[SABER_L][SABER_N] = {0};

    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    int i, j;

    randombytes(seed_A, SABER_SEEDBYTES);
    // for not revealing system RNG state
    shake128(seed_A, SABER_SEEDBYTES, seed_A, SABER_SEEDBYTES);
    randombytes(seed_s, SABER_NOISE_SEEDBYTES);

    GenSecret(s, seed_s);
    MatrixVectorMul_keypair(seed_A, s, b);

    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_N; j++) {
            b[i][j] = (b[i][j] + h1) >> (SABER_EQ - SABER_EP);
        }
    }

    POLVECq2BS(sk, s);
    POLVECp2BS(pk, b);
    memcpy(pk + SABER_POLYVECCOMPRESSEDBYTES, seed_A, sizeof(seed_A));
}

void indcpa_kem_enc(const uint8_t m[SABER_KEYBYTES],
                    const uint8_t seed_sp[SABER_NOISE_SEEDBYTES],
                    const uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES],
                    uint8_t ciphertext[SABER_BYTES_CCA_DEC])
{
    uint16_t sp[SABER_L][SABER_N];
    uint16_t vp[SABER_N] = {0};
    uint16_t message_bit;
    int i, j;
    const uint8_t *seed_A = pk + SABER_POLYVECCOMPRESSEDBYTES;

    GenSecret(sp, seed_sp);
    MatrixVectorMul_encryption(seed_A, sp, ciphertext);

    VectorMul(pk, sp, vp);

    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            message_bit = ((m[j] >> i) & 0x01) << (SABER_EP - 1);
            vp[j * 8 + i] =
                (vp[j * 8 + i] + h1 - message_bit) >> (SABER_EP - SABER_ET);
        }
    }

    POLT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
}

void indcpa_kem_dec(const uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                    const uint8_t ciphertext[SABER_BYTES_CCA_DEC],
                    uint8_t m[SABER_KEYBYTES])
{
    uint16_t s[SABER_L][SABER_N];
    uint16_t v[SABER_N] = {0};
    uint16_t cm[SABER_N];
    int i;

    BS2POLVECq(sk, s);
    VectorMul(ciphertext, s, v);

    BS2POLT(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, cm);

    for (i = 0; i < SABER_N; i++) {
        v[i] = (v[i] + h2 - (cm[i] << (SABER_EP - SABER_ET))) >> (SABER_EP - 1);
    }

    POLmsg2BS(m, v);
}
