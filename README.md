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

## Result

### -O3 Option

When the stack size = 0x2700, malloc will fail. When the stack size = 0x2500, keypair and encapsulation can not run because of stack overflow.

Stack size = 0x2560 is OK.

#### Performance Result

kp cpu cycles: 14241102
enc cpu cycles: 13713843
dec stack overflow
But the results are not constant-time.

#### Code Size Result

with AES and SHA:

```bash
536965908 00000006 T aes256_ctx_release
536937600 00000010 r br_Rcon
536956738 00000024 t aes128_ecb_keyexp.part.0
536956738 00000024 t aes192_ecb_keyexp.part.1
536956738 00000024 t aes256_ecb_keyexp.part.2
536976344 00000032 T SABER_pack_4bit
2147487160 00000032 B kara_tmp
536976376 00000034 T SABER_un_pack4bit
536980456 00000038 T verify
536980752 00000050 T GenCipherText
536977254 00000104 T POLp2BS
536976538 00000114 T BS2POLp
536976222 00000122 T pol_mul
536980158 00000124 T crypto_kem_enc
536976410 00000128 T POLVECp2BS
2147487192 00000128 B kara_tmp_top
536980802 00000148 T ExtractKeyBits
536980006 00000152 T crypto_kem_keypair
536980282 00000174 T crypto_kem_dec
536948380 00000178 T keccak_squeezeblocks
536937392 00000192 r KeccakF_RoundConstants
536965914 00000212 T cbd
536980494 00000258 T cmov
536953774 00000264 T randombytes_init
536976652 00000266 T POLVECq2BS
2147486688 00000280 b shake_op_buf.3015
536979724 00000282 T GenSecret
536977358 00000296 T byte_bank2pol_part
536979392 00000332 T indcpa_kem_dec
536976918 00000336 T BS2POLVECq
536947958 00000422 T keccak_absorb
536978230 00000434 T indcpa_kem_keypair
536977654 00000576 T GenMatrix_poly
536953108 00000666 T AES256_CTR_DRBG_Update
536978664 00000728 T indcpa_kem_enc
536965166 00000742 T aes256_ecb
536948558 00000744 T shake128
536952272 00000836 T randombytes
536949302 00000854 T sha3_256
536974928 00001294 T unrolled_kara_mem_top
536955414 00001324 t br_aes_ct64_ortho
536954038 00001376 t br_aes_ct64_bitslice_Sbox
536963130 00002036 T aes256_ecb_keyexp
536950156 00002116 T sha3_512
536956762 00006368 t aes_ecb4x.constprop.6
536941508 00006450 t KeccakF1600_StatePermute
536966126 00008802 T unrolled_kara_mem_bottom
Total size = 40132 bytes = 39.1914 kB
```

without AES and SHA

```bash
536937600 00000010 r br_Rcon
536976344 00000032 T SABER_pack_4bit
2147487160 00000032 B kara_tmp
536976376 00000034 T SABER_un_pack4bit
536980456 00000038 T verify
536980752 00000050 T GenCipherText
536977254 00000104 T POLp2BS
536976538 00000114 T BS2POLp
536976222 00000122 T pol_mul
536980158 00000124 T crypto_kem_enc
536976410 00000128 T POLVECp2BS
2147487192 00000128 B kara_tmp_top
536980802 00000148 T ExtractKeyBits
536980006 00000152 T crypto_kem_keypair
536980282 00000174 T crypto_kem_dec
536965914 00000212 T cbd
536980494 00000258 T cmov
536953774 00000264 T randombytes_init
536976652 00000266 T POLVECq2BS
536979724 00000282 T GenSecret
536977358 00000296 T byte_bank2pol_part
536979392 00000332 T indcpa_kem_dec
536976918 00000336 T BS2POLVECq
536978230 00000434 T indcpa_kem_keypair
536977654 00000576 T GenMatrix_poly
536978664 00000728 T indcpa_kem_enc
536952272 00000836 T randombytes
536974928 00001294 T unrolled_kara_mem_top
536955414 00001324 t br_aes_ct64_ortho
536954038 00001376 t br_aes_ct64_bitslice_Sbox
536966126 00008802 T unrolled_kara_mem_bottom
Total size = 19006 bytes = 18.5605 kB
```

### -Os Option


#### Performance Result

kp: 6623016
enc: 6930635

#### Code Size

with AES and SHA

```bash
536952270 00000006 T aes256_ctx_release
536952264 00000006 T aes256_ecb
536937600 00000010 r br_Rcon
536951012 00000024 t aes128_ecb_keyexp.part.0
536951012 00000024 t aes192_ecb_keyexp.part.1
536951012 00000024 t aes256_ecb_keyexp.part.2
536958980 00000032 T extract
2147487160 00000032 B kara_tmp
536955224 00000034 T SABER_un_pack4bit
536955186 00000038 T SABER_pack_4bit
536958830 00000042 T verify
536946636 00000048 T AES256_ECB
536958872 00000050 T cmov
536947164 00000052 t br_range_dec32le
536950492 00000052 t br_sub_word
536952480 00000052 T school_book_mul2_16
536959012 00000058 T GenCipherText
536958922 00000058 T floor_special
536952204 00000060 T aes256_ecb_keyexp
536959070 00000096 T ExtractKeyBits
536958428 00000096 T crypto_kem_keypair
536956058 00000104 T POLp2BS
536955392 00000108 T BS2POLp
536955074 00000112 T pol_mul
536946408 00000114 T sha3_256
536946522 00000114 T sha3_512
536958524 00000122 T crypto_kem_enc
536946860 00000122 T randombytes_init
536957998 00000124 T indcpa_kem_dec
536957584 00000126 T VectorMul
536950364 00000128 t br_shift_rows
2147487192 00000128 B kara_tmp_top
536950234 00000130 t br_add_round_key
536956874 00000132 T MatrixVectorMul_keypair
536946042 00000132 T keccak_squeezeblocks
536955258 00000134 T POLVECp2BS
536952050 00000154 t aes_ecb
536946684 00000176 T AES256_CTR_DRBG_Update
536946982 00000182 T randombytes
536958646 00000184 T crypto_kem_dec
536937392 00000192 r KeccakF_RoundConstants
536952276 00000204 T cbd
536957370 00000214 T MatrixVectorMul_encryption
536946174 00000234 T shake128
536955500 00000264 T POLVECq2BS
536949966 00000268 t br_aes_ct64_skey_expand
2147486688 00000280 b shake_op_buf.3015
536949684 00000282 t br_aes_ct64_interleave_in
536956162 00000288 T byte_bank2pol_part
536957710 00000288 T indcpa_kem_enc
536955764 00000294 T BS2POLVECq
536958122 00000306 T GenSecret
536945690 00000352 T keccak_absorb
536957006 00000364 T indcpa_kem_keypair
536956450 00000424 T GenMatrix_poly
536950544 00000468 t br_aes_ct64_keysched
536951036 00001014 t aes_ecb4x
536948590 00001094 t br_aes_ct64_ortho
536952532 00001264 T unrolled_kara_mem_bottom
536953796 00001278 T unrolled_kara_mem_top
536947216 00001374 t br_aes_ct64_bitslice_Sbox
536941498 00004192 t KeccakF1600_StatePermute
Total size = 18358 bytes = 17.9277 kB
```

without AES and SHA

```bash
536937600 00000010 r br_Rcon
536958980 00000032 T extract
2147487160 00000032 B kara_tmp
536955224 00000034 T SABER_un_pack4bit
536955186 00000038 T SABER_pack_4bit
536958830 00000042 T verify
536958872 00000050 T cmov
536947164 00000052 t br_range_dec32le
536950492 00000052 t br_sub_word
536952480 00000052 T school_book_mul2_16
536959012 00000058 T GenCipherText
536958922 00000058 T floor_special
536959070 00000096 T ExtractKeyBits
536958428 00000096 T crypto_kem_keypair
536956058 00000104 T POLp2BS
536955392 00000108 T BS2POLp
536955074 00000112 T pol_mul
536958524 00000122 T crypto_kem_enc
536946860 00000122 T randombytes_init
536957998 00000124 T indcpa_kem_dec
536957584 00000126 T VectorMul
536950364 00000128 t br_shift_rows
2147487192 00000128 B kara_tmp_top
536950234 00000130 t br_add_round_key
536955258 00000134 T POLVECp2BS
536946982 00000182 T randombytes
536958646 00000184 T crypto_kem_dec
536952276 00000204 T cbd
536955500 00000264 T POLVECq2BS
536949966 00000268 t br_aes_ct64_skey_expand
536949684 00000282 t br_aes_ct64_interleave_in
536956162 00000288 T byte_bank2pol_part
536957710 00000288 T indcpa_kem_enc
536955764 00000294 T BS2POLVECq
536958122 00000306 T GenSecret
536957006 00000364 T indcpa_kem_keypair
536956450 00000424 T GenMatrix_poly
536950544 00000468 t br_aes_ct64_keysched
536948590 00001094 t br_aes_ct64_ortho
536952532 00001264 T unrolled_kara_mem_bottom
536953796 00001278 T unrolled_kara_mem_top
536947216 00001374 t br_aes_ct64_bitslice_Sbox
Total size = 10866 bytes = 10.6113 kB
```

## Summary

1. Stack size=0x2560 is OK
2. We find that the keypair and encapsulation are not constant-time and we can not find the newest source code in github.
3. Verify the correction of code_size.sh: 1. run nm and get all the item 2. run clear_nm_out.py for clear results 3. diff
4. O3 Option: 39KB/18KB (with AES and SHA/without)
5. Os Option: 17.9KB/11.6KB (with AES and SHA/without)
6. We found a strange phenomenon: the speed with Os option is faster than O3. 
