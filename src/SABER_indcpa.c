#include "SABER_indcpa.h"

#include <stdint.h>
#include <string.h>

#include "SABER_params.h"
#include "fips202.h"
#include "ntt.h"
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
    int32_t i, j;
    int32_t t1[SABER_N];
    uint16_t messageBit;
    uint16_t t2[SABER_N];
    uint16_t vp[SABER_N] = {0};
    uint16_t b[SABER_L][SABER_N] = {0};
    for (i = 0; i < SABER_L; i++) {
        GenSInTime(t2, seed_sp, i);
        // t1=si in ntt domain
        NTT(t2, t1);
        // MatrixVectorMul
        for (j = 0; j < SABER_L; j++) {
            // b[0]+=a0i*si b[1]+=a1i*si b[2]+=a2i*si
            GenAInTime(t2, seed_A, j, i);
            PolyMulAccFast(t2, t1, b[j]);
        }
        // InnerProduct
        BS2Polp(pk + i * (SABER_EP * SABER_N / 8), t2);
        PolyMulAccFast(t2, t1, vp);
    }
    for (i = 0; i < SABER_L; i++) {
        for (j = 0; j < SABER_N; j++) {
            b[i][j] = (b[i][j] + h1) >> (SABER_EQ - SABER_EP);
        }
        Polp2BS(ciphertext + i * (SABER_EP * SABER_N / 8), b[i]);
    }

    for (j = 0; j < SABER_KEYBYTES; j++) {
        for (i = 0; i < 8; i++) {
            messageBit = ((m[j] >> i) & 0x01);
            messageBit = (messageBit << (SABER_EP - 1));
            vp[j * 8 + i] =
                (vp[j * 8 + i] - messageBit + h1) >> (SABER_EP - SABER_ET);
        }
    }
    PolT2BS(ciphertext + SABER_POLYVECCOMPRESSEDBYTES, vp);
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
