#!/bin/bash

while [[ "$1" != "" ]]
do
    case "$1"
    in
    --aes)      aes="$2";   shift 2;;
    esac
done

if [ "$aes" == "1" ]
then
    riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
    grep ' *Keccak*\| unrolled_\| br_\| PO\| BS\| byte_\| *keccak*\| indcpa_\| Gen\| cbd\| sha\| *aes*\| *Matrix*\| crypto_\| *AES*\| pol_mul\| floor_special\| load\| Vector\| random\| school\| SABER\| cmov\| Extract\| kara\| verify\| store\| extract\| ciphertext_gen' | \
    awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
else
    riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
    grep ' unrolled_\| br_\| PO\| BS\| byte_\| indcpa_\| Gen\| cbd\| crypto_\| pol_mul\| floor_special\| load\| Vector\| random\| school\| SABER\| cmov\| Extract\| kara\| verify\| store\| extract\| ciphertext_gen' | \
    awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
fi