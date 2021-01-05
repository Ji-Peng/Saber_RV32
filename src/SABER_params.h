#include "api.h"

#ifndef PARAMS_H
#    define PARAMS_H

#    define SABER_K 3
#    define SABER_MU 8
#    define SABER_DELTA 3

#    define SABER_N 256
#    define SABER_Q 8192
#    define SABER_P 1024

#    define SABER_NOISEBUFSIZE SABER_MU* SABER_N* SABER_K / 8
#    define SABER_GENMATRIX_ONEVECTOR 13 * SABER_N / 8
#    define SABER_GENMATRIX_BYTE_BANK_LEN \
        SABER_K* SABER_K* SABER_GENMATRIX_ONEVECTOR

#    define SABER_SEEDBYTES 32
#    define SABER_NOISESEEDBYTES 32
#    define SABER_COINBYTES 32
#    define SABER_KEYBYTES 32

#    define SABER_HASHBYTES 32

#    define SABER_POLYBYTES 416  // 13*256/8

#    define SABER_POLYVECBYTES (SABER_K * SABER_POLYBYTES)

#    define SABER_POLYVECCOMPRESSEDBYTES \
        (SABER_K * 320)  // 10*256/8 NOTE : changed till here due to parameter
                         // adaptation

#    define SABER_CIPHERTEXTBYTES (SABER_POLYVECCOMPRESSEDBYTES)

#    define SABER_SCALEBYTES (SABER_DELTA * SABER_N / 8)

#    define SABER_SCALEBYTES_KEM ((SABER_DELTA + 1) * SABER_N / 8)

#    define SABER_INDCPA_PUBLICKEYBYTES \
        (SABER_POLYVECCOMPRESSEDBYTES + SABER_SEEDBYTES)
#    define SABER_INDCPA_SECRETKEYBYTES (SABER_POLYVECBYTES)

#    define SABER_PUBLICKEYBYTES (SABER_INDCPA_PUBLICKEYBYTES)

#    define SABER_SECRETKEYBYTES                                     \
        (SABER_INDCPA_SECRETKEYBYTES + SABER_INDCPA_PUBLICKEYBYTES + \
         SABER_HASHBYTES + SABER_KEYBYTES)

#    define SABER_BYTES_CCA_DEC         \
        (SABER_POLYVECCOMPRESSEDBYTES + \
         SABER_SCALEBYTES_KEM) /* Second part is for Targhi-Unruh */

#endif
