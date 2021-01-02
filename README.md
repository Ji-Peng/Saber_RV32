# Saber on RV32IMAC

## Experimental Setup

[SiFive FE310 RISC-V CPU](https://www.sifive.com/boards/hifive1-rev-b)

bsp: board support package

freedom-metal: hardware interface

scripts: extra tools

src: source file

## Upload and Debug

./jlink.sh --hex out/bench.hex --jlink JLinkExe

./jlink.sh --elf out/bench.elf --jlink JLinkGDBServer --gdb riscv64-unknown-elf-gdb

and then input "load"

## GDBServer Tutorial

[Segger JLink-GDBServer](https://wiki.segger.com/J-Link_GDB_Server)

## Code Size

including AES and Keccake Implementation:

```bash
riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
grep ' crypto_\| load_\| sha3_\| poly_\| byte_\| indcpa_\| cbd\| randombytes\| PO\| BS\| Inner\| Gen\| shake\| verify\| cmov\| school\| *keccak*\| *Keccak*\| *kara*\| *toom*\| *Matrix*\| *Vector*\| *aes*\| *AES*' | \
awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
```

excluding AES and Keccake Implementation:

```bash
riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
grep ' crypto_\| load_\| poly_\| byte_\| indcpa_\| cbd\| randombytes\| PO\| BS\| Inner\| Gen\| verify\| cmov\| school\| *kara*\| *toom*\| *Matrix*\| *Vector' | \
awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
```

### After using optimization in Saber on ARM for Cortex-M0

#### O3 without AES&SHA3

```bash
2147486462 00000002 b poly_complete.2745
536976814 00000032 T POLT2BS
2147487160 00000032 B kara_tmp
536976846 00000034 T BS2POLT
536979692 00000036 T verify
536977412 00000042 T BS2POLVECq
536978746 00000076 T GenSecret
536977844 00000104 T POLp2BS
536977730 00000114 T BS2POLp
536977454 00000116 T POLVECp2BS
536980138 00000124 T crypto_kem_enc
2147487192 00000128 B kara_tmp_top
536979986 00000152 T crypto_kem_keypair
536977570 00000160 T POLmsg2BS
536965888 00000166 T cbd
536980262 00000176 T crypto_kem_dec
536978568 00000178 T indcpa_kem_dec
536976630 00000184 T VectorMul
536976150 00000230 T MatrixVectorMul_keypair
536976380 00000250 T MatrixVectorMul_encryption
536977160 00000252 T POLVECq2BS
536979728 00000258 T cmov
536976880 00000280 T BS2POLq
536953750 00000282 T randombytes_init
536978822 00000296 T byte_bank2pol_part
536977948 00000306 T indcpa_kem_keypair
536978254 00000314 T indcpa_kem_enc
536979118 00000574 T GenMatrix_poly
536952248 00000836 T randombytes
536974856 00001294 T unrolled_kara_mem_top
536966054 00008802 T unrolled_kara_mem_bottom
Total size = 15830 bytes = 15.459 kB
```

#### O3 with AES&SHA3

```bash
2147486462 00000002 b poly_complete.2745
536965882 00000006 T aes256_ctx_release
536976814 00000032 T POLT2BS
2147487160 00000032 B kara_tmp
536976846 00000034 T BS2POLT
536979692 00000036 T verify
536977412 00000042 T BS2POLVECq
536978746 00000076 T GenSecret
536977844 00000104 T POLp2BS
536977730 00000114 T BS2POLp
536977454 00000116 T POLVECp2BS
536980138 00000124 T crypto_kem_enc
2147487192 00000128 B kara_tmp_top
536979986 00000152 T crypto_kem_keypair
536977570 00000160 T POLmsg2BS
536965888 00000166 T cbd
536980262 00000176 T crypto_kem_dec
536978568 00000178 T indcpa_kem_dec
536948356 00000178 T keccak_squeezeblocks
536976630 00000184 T VectorMul
536937376 00000192 r KeccakF_RoundConstants
536976150 00000230 T MatrixVectorMul_keypair
536976380 00000250 T MatrixVectorMul_encryption
536977160 00000252 T POLVECq2BS
536979728 00000258 T cmov
536976880 00000280 T BS2POLq
2147486688 00000280 b shake_op_buf.2736
536953750 00000282 T randombytes_init
536978822 00000296 T byte_bank2pol_part
536977948 00000306 T indcpa_kem_keypair
536978254 00000314 T indcpa_kem_enc
536947934 00000422 T keccak_absorb
536979118 00000574 T GenMatrix_poly
536953084 00000666 T AES256_CTR_DRBG_Update
536965140 00000742 T aes256_ecb
536948534 00000744 T shake128
536952248 00000836 T randombytes
536949278 00000854 T sha3_256
536974856 00001294 T unrolled_kara_mem_top
536955408 00001324 t br_aes_ct64_ortho
536954032 00001376 t br_aes_ct64_bitslice_Sbox
536963100 00002040 T aes256_ecb_keyexp
536950132 00002116 T sha3_512
536956732 00006368 t aes_ecb4x.constprop.3
536941484 00006450 t KeccakF1600_StatePermute
536966054 00008802 T unrolled_kara_mem_bottom
Total size = 39588 bytes = 38.6602 kB
```

#### Os with AES&SHA3

```bash
2147486462 00000002 b poly_complete.2745
536952242 00000006 T aes256_ctx_release
536952236 00000006 T aes256_ecb
2147487160 00000032 B kara_tmp
536955572 00000034 T BS2POLT
536955534 00000038 T POLT2BS
536958024 00000040 T verify
536956142 00000042 T BS2POLVECq
536946612 00000048 T AES256_ECB
536958064 00000050 T cmov
536952388 00000052 T school_book_mul2_16
536952170 00000066 T aes256_ecb_keyexp
536957236 00000076 T GenSecret
536956310 00000076 T POLmsg2BS
536958114 00000096 T crypto_kem_keypair
536955436 00000098 T VectorMul
536956494 00000104 T POLp2BS
536956386 00000108 T BS2POLp
536946384 00000114 T sha3_256
536946498 00000114 T sha3_512
536958210 00000124 T crypto_kem_enc
536956184 00000126 T POLVECp2BS
2147487192 00000128 B kara_tmp_top
536955094 00000132 T MatrixVectorMul_keypair
536946018 00000132 T keccak_squeezeblocks
536946836 00000136 T randombytes_init
536952248 00000140 T cbd
536956902 00000152 T indcpa_kem_enc
536952016 00000154 t aes_ecb
536946660 00000176 T AES256_CTR_DRBG_Update
536957054 00000182 T indcpa_kem_dec
536946972 00000182 T randombytes
536958334 00000186 T crypto_kem_dec
536937376 00000192 r KeccakF_RoundConstants
536955226 00000210 T MatrixVectorMul_encryption
536946150 00000234 T shake128
536955886 00000256 T POLVECq2BS
536949956 00000268 t br_aes_ct64_skey_expand
536955606 00000280 T BS2POLq
2147486688 00000280 b shake_op_buf.2736
536949674 00000282 t br_aes_ct64_interleave_in
536957312 00000288 T byte_bank2pol_part
536956598 00000304 T indcpa_kem_keypair
536945666 00000352 T keccak_absorb
536957600 00000424 T GenMatrix_poly
536950534 00000468 t br_aes_ct64_keysched
536951002 00001014 t aes_ecb4x
536948580 00001094 t br_aes_ct64_ortho
536952440 00001264 T unrolled_kara_mem_bottom
536953704 00001278 T unrolled_kara_mem_top
536947206 00001374 t br_aes_ct64_bitslice_Sbox
536941474 00004192 t KeccakF1600_StatePermute
Total size = 17206 bytes = 16.8027 kB
```

#### Os without AES&SHA3

```bash
2147486462 00000002 b poly_complete.2745
2147487160 00000032 B kara_tmp
536955572 00000034 T BS2POLT
536955534 00000038 T POLT2BS
536958024 00000040 T verify
536956142 00000042 T BS2POLVECq
536958064 00000050 T cmov
536952388 00000052 T school_book_mul2_16
536957236 00000076 T GenSecret
536956310 00000076 T POLmsg2BS
536958114 00000096 T crypto_kem_keypair
536955436 00000098 T VectorMul
536956494 00000104 T POLp2BS
536956386 00000108 T BS2POLp
536958210 00000124 T crypto_kem_enc
536956184 00000126 T POLVECp2BS
2147487192 00000128 B kara_tmp_top
536955094 00000132 T MatrixVectorMul_keypair
536946836 00000136 T randombytes_init
536952248 00000140 T cbd
536956902 00000152 T indcpa_kem_enc
536957054 00000182 T indcpa_kem_dec
536946972 00000182 T randombytes
536958334 00000186 T crypto_kem_dec
536955226 00000210 T MatrixVectorMul_encryption
536955886 00000256 T POLVECq2BS
536955606 00000280 T BS2POLq
536957312 00000288 T byte_bank2pol_part
536956598 00000304 T indcpa_kem_keypair
536957600 00000424 T GenMatrix_poly
536952440 00001264 T unrolled_kara_mem_bottom
536953704 00001278 T unrolled_kara_mem_top
Total size = 6640 bytes = 6.48438 kB
```
