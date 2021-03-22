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
    uint16_t b[SABER_L][SABER_N] = {0};

    uint8_t seed_A[SABER_SEEDBYTES];
    uint8_t seed_s[SABER_NOISE_SEEDBYTES];
    int i, j;

    RandomBytes(seed_A, SABER_SEEDBYTES);
    // for not revealing system RNG state
    shake128(seed_A, SABER_SEEDBYTES, seed_A, SABER_SEEDBYTES);
    RandomBytes(seed_s, SABER_NOISE_SEEDBYTES);

    MatrixVectorMulKP(seed_A, seed_s, sk, b);

    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_N; j++) {
            b[i][j] = (b[i][j] + h1) >> (SABER_EQ - SABER_EP);
        }
    }

    PolVecp2BS(pk, b);
    memcpy(pk + SABER_POLYVECCOMPRESSEDBYTES, seed_A,
           SABER_SEEDBYTES * sizeof(uint8_t));
}

// fast matrix-vector mul and innerproduct
// #define ENC_FAST

void indcpa_kem_enc(const uint8_t m[SABER_KEYBYTES],
                    const uint8_t seed_sp[SABER_NOISE_SEEDBYTES],
                    const uint8_t pk[SABER_INDCPA_PUBLICKEYBYTES],
                    uint8_t ciphertext[SABER_BYTES_CCA_DEC])
{
    const uint8_t *seed_A = pk + SABER_POLYVECCOMPRESSEDBYTES;
#ifdef FASTGENA_SLOWMUL
    uint16_t sp[SABER_L][SABER_N];
    GenSecret(sp, seed_sp);
    MatrixVectorMulEnc(seed_A, sp, ciphertext);
    InnerProdInTimeEnc(pk, sp, ciphertext, m);
#elif defined(FASTGENA_FASTMUL)
    // save s in ntt domain for fast computation
    int32_t sp[SABER_L][SABER_N];
    GenSecretNTT(sp, seed_sp);
    MatrixVectorMulEnc(seed_A, sp, ciphertext);
    InnerProdInTimeEnc(pk, sp, ciphertext, m);

#else
#endif
}

void indcpa_kem_dec(const uint8_t sk[SABER_INDCPA_SECRETKEYBYTES],
                    const uint8_t ciphertext[SABER_BYTES_CCA_DEC],
                    uint8_t m[SABER_KEYBYTES])
{
    uint16_t v[SABER_N] = {0};
    uint16_t cm[SABER_N];
    int i;

    InnerProdInTimeDec(ciphertext, sk, v);

    BS2PolT(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, cm);

    for (i = 0; i < SABER_N; i++) {
        v[i] = (v[i] + h2 - (cm[i] << (SABER_EP - SABER_ET))) >> (SABER_EP - 1);
    }

    PolMsg2BS(m, v);
}
