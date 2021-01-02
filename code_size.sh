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
    grep ' crypto_\| load_\| sha3_\| poly_\| byte_\| indcpa_\| cbd\| randombytes\| PO\| BS\| Inner\| Gen\| shake\| verify\| cmov\| school\| *keccak*\| *Keccak*\| *kara*\| *toom*\| *Matrix*\| *Vector*\| *aes*\| *AES*' | \
    awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
else
    riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
    grep ' crypto_\| load_\| poly_\| byte_\| indcpa_\| cbd\| randombytes\| PO\| BS\| Inner\| Gen\| verify\| cmov\| school\| *kara*\| *toom*\| *Matrix*\| *Vector' | \
    awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
fi


