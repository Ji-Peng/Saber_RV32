#ifndef PARAMS_H
#define PARAMS_H

/* Change this for different security strengths */
// #define SABER_L 2 /* LightSaber */
#define SABER_L 3 /* Saber */
// #define SABER_L 4 /* FireSaber */

#if SABER_L == 2
#    define SABER_MU 10
#    define SABER_ET 3
#    define FASTGENA_FASTMUL
// #    define FIVE_LAYER_NTT
#    define SIX_LAYER_NTT
// #    define SEVEN_LAYER_NTT
// #    define COMPLETE_NTT
#elif SABER_L == 3
#    define SABER_MU 8
#    define SABER_ET 4
#    define SLOWGENA_FASTMUL
// #    define FIVE_LAYER_NTT
#    define SIX_LAYER_NTT
// #    define SEVEN_LAYER_NTT
// #    define COMPLETE_NTT
#elif SABER_L == 4
#    define SABER_MU 6
#    define SABER_ET 6
#    define FASTGENA_SLOWMUL
// #    define FIVE_LAYER_NTT
#    define SIX_LAYER_NTT
// #    define SEVEN_LAYER_NTT
// #    define COMPLETE_NTT
#endif

#    define NTTASM

// check computation strategy
#if !defined(FASTGENA_SLOWMUL) && !defined(FASTGENA_FASTMUL) && \
    !defined(SLOWGENA_FASTMUL)
#    error \
        "Please define FASTGENA_SLOWMUL or FASTGENA_FASTMUL or SLOWGENA_FASTMUL"
#endif

#define SABER_EQ 13
#define SABER_EP 10
#define SABER_N 256

#define SABER_SEEDBYTES 32
#define SABER_NOISE_SEEDBYTES 32
#define SABER_KEYBYTES 32
#define SABER_HASHBYTES 32

#define SABER_POLYCOINBYTES (SABER_MU * SABER_N / 8)

#define SABER_POLYBYTES (SABER_EQ * SABER_N / 8)
#define SABER_POLYVECBYTES (SABER_L * SABER_POLYBYTES)

#define SABER_POLYCOMPRESSEDBYTES (SABER_EP * SABER_N / 8)
#define SABER_POLYVECCOMPRESSEDBYTES (SABER_L * SABER_POLYCOMPRESSEDBYTES)

#define SABER_SCALEBYTES_KEM (SABER_ET * SABER_N / 8)

#define SABER_INDCPA_PUBLICKEYBYTES \
    (SABER_POLYVECCOMPRESSEDBYTES + SABER_SEEDBYTES)
// #define SABER_INDCPA_SECRETKEYBYTES (SABER_POLYVECBYTES)

#define SABER_SKPOLYBYTES 4 * SABER_N / 8
#define SABER_INDCPA_SECRETKEYBYTES SABER_L* SABER_SKPOLYBYTES

#define SABER_PUBLICKEYBYTES (SABER_INDCPA_PUBLICKEYBYTES)
#define SABER_SECRETKEYBYTES                                     \
    (SABER_INDCPA_SECRETKEYBYTES + SABER_INDCPA_PUBLICKEYBYTES + \
     SABER_HASHBYTES + SABER_KEYBYTES)

#define SABER_BYTES_CCA_DEC \
    (SABER_POLYVECCOMPRESSEDBYTES + SABER_SCALEBYTES_KEM)

#define h1 (1 << (SABER_EQ - SABER_EP - 1))
#define h2                                                      \
    ((1 << (SABER_EP - 2)) - (1 << (SABER_EP - SABER_ET - 1)) + \
     (1 << (SABER_EQ - SABER_EP - 1)))
#endif
