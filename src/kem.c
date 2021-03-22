#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "SABER_indcpa.h"
#include "SABER_params.h"
#include "api.h"
#include "fips202.h"
#include "rng.h"
#include "verify.h"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
    int i;
    // sk[0:SABER_INDCPA_SECRETKEYBYTES-1] <-- sk
    indcpa_kem_keypair(pk, sk);
    // sk[SABER_INDCPA_SECRETKEYBYTES:SABER_INDCPA_SECRETKEYBYTES+SABER_INDCPA_PUBLICKEYBYTES-1<--pk
    for (i = 0; i < SABER_INDCPA_PUBLICKEYBYTES; i++)
        sk[i + SABER_INDCPA_SECRETKEYBYTES] = pk[i];

    // Then hash(pk) is appended.
    sha3_256(sk + SABER_SECRETKEYBYTES - 64, pk, SABER_INDCPA_PUBLICKEYBYTES);

    // Remaining part of sk contains a pseudo-random number. This is output when
    // check in crypto_kem_dec() fails.
    RandomBytes(sk + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES);
    return (0);
}

int crypto_kem_enc(unsigned char *c, unsigned char *k, const unsigned char *pk)
{
    // Will contain key, coins
    unsigned char kr[64];
    unsigned char buf[64];

    RandomBytes(buf, 32);

    // BUF[0:31] <-- random message (will be used as the key for
    // client) Note: hash doesnot release system RNG output
    sha3_256(buf, buf, 32);

    // BUF[32:63] <-- Hash(public key); Multitarget countermeasure for
    // coins + contributory KEM
    sha3_256(buf + 32, pk, SABER_INDCPA_PUBLICKEYBYTES);

    // kr[0:63] <-- Hash(buf[0:63]);
    // K^ <-- kr[0:31]
    // noiseseed (r) <-- kr[32:63];
    sha3_512(kr, buf, 64);
    // buf[0:31] contains message; kr[32:63] contains randomness r;
    indcpa_kem_enc(buf, kr + 32, pk, c);

    sha3_256(kr + 32, c, SABER_BYTES_CCA_DEC);

    // hash concatenation of pre-k and h(c) to k
    sha3_256(k, kr, 64);

    return (0);
}

int crypto_kem_dec(unsigned char *k, const unsigned char *c,
                   const unsigned char *sk)
{
    int i, fail;
    unsigned char cmp[SABER_BYTES_CCA_DEC];
    unsigned char buf[64];
    // Will contain key, coins
    unsigned char kr[64];
    const unsigned char *pk = sk + SABER_INDCPA_SECRETKEYBYTES;

    // buf[0:31] <-- message
    indcpa_kem_dec(sk, c, buf);

    // Multitarget countermeasure for coins + contributory KEM
    // Save hash by storing h(pk) in sk
    for (i = 0; i < 32; i++)
        buf[32 + i] = sk[SABER_SECRETKEYBYTES - 64 + i];

    sha3_512(kr, buf, 64);

    indcpa_kem_enc(buf, kr + 32, pk, cmp);

    fail = Verify(c, cmp, SABER_BYTES_CCA_DEC);

    // overwrite coins in kr with h(c)
    sha3_256(kr + 32, c, SABER_BYTES_CCA_DEC);

    CMov(kr, sk + SABER_SECRETKEYBYTES - SABER_KEYBYTES, SABER_KEYBYTES, fail);

    // hash concatenation of pre-k and h(c) to k
    sha3_256(k, kr, 64);

    return (0);
}
