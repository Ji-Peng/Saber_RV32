.text

# M * Mprime = 1 mod R (R=2^32), M and Mprime are used for montgomery reduce
.equ Mprime, 0xff9ff801
.equ M, 0xa00801
# v = (((int64_t)1 << 48) + M / 2) / M is used for barrett reduce
.equ V, 0x199851d
# NINV = R^2 * (1/32) mod M is used for reverting standard domain
.equ NINV, 0x4b4686
# M/2
.equ MDiv2, 0x500400

# int64_t a=in1*in2
# int32_t t=a*Mprime
# t=((int64_t)a-(int64_t)t*M)>>32
.macro MontMul in1, in2, out, CM, CMprime, tmp1
    # out=low(in1*in2)=aLo
    mul \out, \in1, \in2
    # aLo=aLo*M'
    mul \out, \out, \CMprime
    # aLo=aLo*M, only high-libm is needed
    mulh \out, \out, \CM
    # tmp1=high(in1*in2)=aHi
    mulh \tmp1, \in1, \in2
    # out=aHi-aLo
    sub \out, \tmp1, \out
.endm

# int64_t a=in1*in2
# int32_t t=a*Mprime
# t=((int64_t)a-(int64_t)t*M)>>32
# in1 will be overwritten, in2 will not be overwritten
.macro MontMulNew in1, in2, out, CM, CMprime
    # out=low(in1*in2)=aLo
    mul \out, \in1, \in2
    # aLo=aLo*M'
    mul \out, \out, \CMprime
    # aLo=aLo*M, only high-libm is needed
    mulh \out, \out, \CM
    # in1=high(in1*in2)=aHi
    mulh \in1, \in1, \in2
    # out=aHi-aLo
    sub \out, \in1, \out
.endm

# int64_t a=inHi&&inLo
# inHi and inLo wille be rewrited
# t=(a mod R)*M' mod R, a=(a-t*M)/R
.macro MontReduce inLo, inHi, out, CM, CMprime
    # inLo=inLo*M'
    mul \inLo, \inLo, \CMprime
    # inLo=inLo*M only high-limb is needed, because the low-limb is same
    mulh \inLo, \inLo, \CM
    # out=inHi-inLo
    sub \out, \inHi, \inLo
.endm

# input: int32_t a
# int32_t t=((int64_t)v * a + ((int64_t)1 << 47)) >> 48
# return a-t*M;
.macro BarrettReduce in, out, tmp1
    # tmp1=V
    li \tmp1, V
    # tmp1=high(v*a)
    mulh \tmp1, \tmp1, \in
    # out=(1<<47)>>32=1<<15, 15=12+3
    lui \out, 0x8
    # (v*a+1<<47)>>32
    add \out, \out, \tmp1
    # (v*a+1<<47)>>48
    srai \out, \out, 0x10
    # t*M
    li \tmp1, M
    mul \out, \out, \tmp1
    # a-t*M
    sub \out, \in, \out
.endm

# Butterfly Unit for NTT
# in1, in2: t=zeta*in2, in2=in1-t; in1=in1+t;
.macro BFU in1, in2, zeta, CM, CMprime, tmp1
    # tmp1=zeta*in1
    MontMulNew \in2, \zeta, \tmp1, \CM, \CMprime
    # in2=in1-t
    sub \in2, \in1, \tmp1
    # in1=in1+t
    add \in1, \in1, \tmp1
.endm

# Butterfly Unit for INTT
# in1, in2: t=in1-in2, in1=in1+in2, in2=t*zeta
.macro BFUINTT in1, in2, zeta, CM, CMprime, tmp1
    sub \tmp1, \in1, \in2
    add \in1, \in1, \in2
    MontMulNew \tmp1, \zeta, \in2, \CM, \CMprime
.endm

# int32_t mask;
# mask = (a + M / 2) >> 31;
# a += mask & M;
# mask = a - M / 2;
# mask = (mask >> 31) & 0x1;
# mask = mask - 1;
# a -= mask & M;
.macro CenReduce in, out, CM, CMDiv2, tmp1
    # tmp1=in+M/2
    add \tmp1, \in, \CMDiv2
    # tmp1>>31 : 0xffff_ffff or 0
    srai \tmp1, \tmp1, 0x1f
    # tmp1=tmp1 & M
    and \tmp1, \tmp1, \CM
    # in=in+tmp1
    add \in, \in, \tmp1
    # tmp1=in-M/2
    sub \tmp1, \in, \CMDiv2
    # tmp1>>31 : 0 or 1
    srli \tmp1, \tmp1, 0x1f
    # tmp1 = tmp1 - 1 : 0xffff_ffff or 0
    addi \tmp1, \tmp1, -1
    # tmp1=tmp1&M
    and \tmp1, \tmp1, \CM
    # out=in-tmp1
    sub \out, \in, \tmp1
.endm

# revert to Standard domain and reduce to Centered representation
.macro SCReduce CNINV, CM, CMprime, CMDiv2, tmp1, tmp2
    # reduce s0
    MontMulNew s0, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s0, \CM, \CMDiv2, \tmp2
    # reduce s1
    MontMulNew s1, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s1, \CM, \CMDiv2, \tmp2
    # reduce s2
    MontMulNew s2, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s2, \CM, \CMDiv2, \tmp2
    # reduce s3
    MontMulNew s3, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s3, \CM, \CMDiv2, \tmp2
    # reduce s4
    CenReduce s4, s4, \CM, \CMDiv2, \tmp2
    # reduce s5
    CenReduce s5, s5, \CM, \CMDiv2, \tmp2
    # reduce s6
    CenReduce s6, s6, \CM, \CMDiv2, \tmp2
    # reduce s7
    CenReduce s7, s7, \CM, \CMDiv2, \tmp2
.endm

.macro LoadZetas
    # t0-t6 save 7 zetas
    lw t0, 0*4(a2)
    lw t1, 1*4(a2)
    lw t2, 2*4(a2)
    lw t3, 3*4(a2)
    lw t4, 4*4(a2)
    lw t5, 5*4(a2)
    lw t6, 6*4(a2)
.endm

.macro LoadS hw, len, wordLen, reg
    l\hw s0, \len*\wordLen*0(\reg)
    l\hw s1, \len*\wordLen*1(\reg)
    l\hw s2, \len*\wordLen*2(\reg)
    l\hw s3, \len*\wordLen*3(\reg)
    l\hw s4, \len*\wordLen*4(\reg)
    l\hw s5, \len*\wordLen*5(\reg)
    l\hw s6, \len*\wordLen*6(\reg)
    l\hw s7, \len*\wordLen*7(\reg)
.endm

.macro LoadS_2layer hw, len, wordLen, reg
    l\hw s0, \len*\wordLen*0(\reg)
    l\hw s1, \len*\wordLen*1(\reg)
    l\hw s2, \len*\wordLen*2(\reg)
    l\hw s3, \len*\wordLen*3(\reg)

    l\hw s4, \len*\wordLen*0+4*1(\reg)
    l\hw s5, \len*\wordLen*1+4*1(\reg)
    l\hw s6, \len*\wordLen*2+4*1(\reg)
    l\hw s7, \len*\wordLen*3+4*1(\reg)

    l\hw t0, \len*\wordLen*0+4*2(\reg)
    l\hw t1, \len*\wordLen*1+4*2(\reg)
    l\hw t2, \len*\wordLen*2+4*2(\reg)
    l\hw t3, \len*\wordLen*3+4*2(\reg)

    l\hw t4, \len*\wordLen*0+4*3(\reg)
    l\hw t5, \len*\wordLen*1+4*3(\reg)
    l\hw t6, \len*\wordLen*2+4*3(\reg)
    l\hw s11,\len*\wordLen*3+4*3(\reg)
.endm

.macro StoreS len, reg
    sw s0, \len*4*0(\reg)
    sw s1, \len*4*1(\reg)
    sw s2, \len*4*2(\reg)
    sw s3, \len*4*3(\reg)
    sw s4, \len*4*4(\reg)
    sw s5, \len*4*5(\reg)
    sw s6, \len*4*6(\reg)
    sw s7, \len*4*7(\reg)
.endm

.macro StoreS_2layer len, reg
    sw s0, \len*4*0(\reg)
    sw s1, \len*4*1(\reg)
    sw s2, \len*4*2(\reg)
    sw s3, \len*4*3(\reg)

    sw s4, \len*4*0+4*1(\reg)
    sw s5, \len*4*1+4*1(\reg)
    sw s6, \len*4*2+4*1(\reg)
    sw s7, \len*4*3+4*1(\reg)

    sw t0, \len*4*0+4*2(\reg)
    sw t1, \len*4*1+4*2(\reg)
    sw t2, \len*4*2+4*2(\reg)
    sw t3, \len*4*3+4*2(\reg)

    sw t4, \len*4*0+4*3(\reg)
    sw t5, \len*4*1+4*3(\reg)
    sw t6, \len*4*2+4*3(\reg)
    sw s11,\len*4*3+4*3(\reg)
.endm

# {hi,lo}=in1*in2
.macro MUL64 in1, in2, lo, hi
    mul \lo, \in1, \in2
    mulh \hi, \in1, \in2
.endm

# inLo and inHi will be rewrited
.macro ADD64 inLo, inHi, outLo, outHi
    # add low 32bits
    add  \outLo, \outLo, \inLo
    # (inLo+outLo)<inLo=>carry=1 else carry=0
    sltu \inLo, \outLo, \inLo
    # add high 32bits
    add  \outHi, \outHi, \inHi
    # add carry
    add  \outHi, \outHi, \inLo
.endm

# void ntt_asm(const uint16_t in[256], int32_t out[256],
# const int32_t rootTable[31]);

# register usage for 3-layer merging: a0-a2: parameter, t0-t6: zetas
# s0-s7: 8 coefficients s8/s9: temp, a6/a7: M/Mprime, a4/a5: looper control

# register usage for 2-layer merging: a0-a2: parameter, s0-s7+t0-t6-s11: 16 coefficients
# a3/a4/a5: 3 zetas, a6/a7: M/Mprime, s8: temp, s9/s10: looper control
.globl ntt_asm_5layer
.align 2
ntt_asm_5layer:
    addi sp, sp, -4*12
    sw s0, 0*4(sp)
    sw s1, 1*4(sp)
    sw s2, 2*4(sp)
    sw s3, 3*4(sp)
    sw s4, 4*4(sp)
    sw s5, 5*4(sp)
    sw s6, 6*4(sp)
    sw s7, 7*4(sp)
    sw s8, 8*4(sp)
    sw s9, 9*4(sp)
    sw s10,10*4(sp)
    sw s11,11*4(sp)

    # zeta, M, Mprime
    LoadZetas
    li a6, M
    li a7, Mprime
    # loop control, 32: loop numbers, 2: uint16_t=2B
    addi a4, a0, 32*2
# 123 layers, input is uint16_t in[N], output is int32_t out[N]
looper_123:
    # s0-s7 = a[0,32,64,...,224]
    LoadS h, 32, 2, a0
    # level1
    BFU s0, s4, t0, a6, a7, s8
    BFU s1, s5, t0, a6, a7, s8
    BFU s2, s6, t0, a6, a7, s8
    BFU s3, s7, t0, a6, a7, s8
    # level2
    BFU s0, s2, t1, a6, a7, s8
    BFU s1, s3, t1, a6, a7, s8
    BFU s4, s6, t2, a6, a7, s8
    BFU s5, s7, t2, a6, a7, s8
    # level3
    BFU s0, s1, t3, a6, a7, s8
    BFU s2, s3, t4, a6, a7, s8
    BFU s4, s5, t5, a6, a7, s8
    BFU s6, s7, t6, a6, a7, s8
    //save results
    StoreS 32, a1

    addi a0, a0, 2
    addi a1, a1, 4
    bne a4, a0, looper_123

# ============== level 4-5 ============== #
# reset a1, 32: inner loop number, 4: wordLen
addi a1, a1, -32*4
# a2 points to next zetas
addi a2, a2, 4*7
# set outer loop control register s9, 8: inner loop number
addi s9, a2, 8*4*3

outer_45:
    # load 3 zetas
    lw a3, 0*4(a2)
    lw a4, 1*4(a2)
    lw a5, 2*4(a2)
    # set looper control register s10, inner loop's increment is 4 words
    addi s10, a1, 2*4*4

    inner_45:
        LoadS_2layer w, 8, 4, a1
        # layer 4
        BFU s0, s2, a3, a6, a7, s8
        BFU s1, s3, a3, a6, a7, s8
        # layer 5
        BFU s0, s1, a4, a6, a7, s8
        BFU s2, s3, a5, a6, a7, s8

        # layer 4
        BFU s4, s6, a3, a6, a7, s8
        BFU s5, s7, a3, a6, a7, s8
        # layer 5
        BFU s4, s5, a4, a6, a7, s8
        BFU s6, s7, a5, a6, a7, s8

        # layer 4
        BFU t0, t2, a3, a6, a7, s8
        BFU t1, t3, a3, a6, a7, s8
        # layer 5
        BFU t0, t1, a4, a6, a7, s8
        BFU t2, t3, a5, a6, a7, s8

        # layer 4
        BFU t4, t6, a3, a6, a7, s8
        BFU t5, s11,a3, a6, a7, s8
        # layer 5
        BFU t4, t5, a4, a6, a7, s8
        BFU t6, s11,a5, a6, a7, s8
        //save results
        StoreS_2layer 8, a1
        # looper control register changes
        addi a1, a1, 4*4
        bne s10, a1, inner_45
    # a1 points to next block, a2 points to next 3 zetas
    # each block has 32 words, inner loop's increment is 4 words
    addi a1, a1, 32*4-2*4*4
    addi a2, a2, 4*3
    bne  s9, a2, outer_45

    lw s0, 0*4(sp)
    lw s1, 1*4(sp)
    lw s2, 2*4(sp)
    lw s3, 3*4(sp)
    lw s4, 4*4(sp)
    lw s5, 5*4(sp)
    lw s6, 6*4(sp)
    lw s7, 7*4(sp)
    lw s8, 8*4(sp)
    lw s9, 9*4(sp)
    lw s10,10*4(sp)
    lw s11,11*4(sp)
    addi sp, sp, 4*12
    ret

# void intt_asm(const uint32_t in[256], int32_t out[256],
# const int32_t rootTable[31]);

# register usage for 2-layer merging: 
# a0-a2: parameter, a3/a4: looper control, a5/a6: M/Mprime
# s0-s7+t0-t6+s11: 16 coefficients, a7/s9/s10: 3 zetas, s8: temp

# register usage for 3-layer merging: 
# a0-a2: parameter, a3/a4: looper control, a5/a6: M/Mprime, s11: NINV, s10: M/2
# t0-t6: 7 zetas, s0-s7: 8 coefficients, s8-s9: temp
.globl intt_asm_5layer
.align 2
intt_asm_5layer:
    addi sp, sp, -4*12
    sw s0, 0*4(sp)
    sw s1, 1*4(sp)
    sw s2, 2*4(sp)
    sw s3, 3*4(sp)
    sw s4, 4*4(sp)
    sw s5, 5*4(sp)
    sw s6, 6*4(sp)
    sw s7, 7*4(sp)
    sw s8, 8*4(sp)
    sw s9, 9*4(sp)
    sw s10,10*4(sp)
    sw s11,11*4(sp)

# set outer loop control, 8: inner loop number
addi a3, a2, 8*4*3
li a5, M
li a6, Mprime
outer_12:
    # load 3 zetas
    lw a7, 0*4(a2)
    lw s9, 1*4(a2)
    lw s10,2*4(a2)
    # set looper control
    addi a4, a1, 2*4*4

    inner_12:
        LoadS_2layer w, 8, 4, a0
        # layer 1
        BFUINTT s0, s1, a7, a5, a6, s8
        BFUINTT s2, s3, s9, a5, a6, s8
        # layer 2
        BFUINTT s0, s2, s10, a5, a6, s8
        BFUINTT s1, s3, s10, a5, a6, s8

        # layer 1
        BFUINTT s4, s5, a7, a5, a6, s8
        BFUINTT s6, s7, s9, a5, a6, s8
        # layer 2
        BFUINTT s4, s6, s10, a5, a6, s8
        BFUINTT s5, s7, s10, a5, a6, s8

        # layer 1
        BFUINTT t0, t1, a7, a5, a6, s8
        BFUINTT t2, t3, s9, a5, a6, s8
        # layer 2
        BFUINTT t0, t2, s10, a5, a6, s8
        BFUINTT t1, t3, s10, a5, a6, s8

        # layer 1
        BFUINTT t4, t5, a7, a5, a6, s8
        BFUINTT t6, s11,s9, a5, a6, s8
        # layer 2
        BFUINTT t4, t6, s10, a5, a6, s8
        BFUINTT t5, s11,s10, a5, a6, s8                        
        //save results
        StoreS_2layer 8, a1
        # looper control register changes
        addi a0, a0, 4*4
        addi a1, a1, 4*4
        bne  a1, a4, inner_12
    # a1 points to next block, a2 points to next 3 zetas
    addi a0, a0, 32*4-2*4*4
    addi a1, a1, 32*4-2*4*4
    addi a2, a2, 4*3
    bne  a2, a3, outer_12

    # reset a1
    addi a1, a1, -8*32*4
    # loop control, 32: loop numbers, 4: int32_t=4B
    addi a3, a1, 32*4
    LoadZetas
    # Load MDiv2 and NINV for reverting to standard domain
    li s10, MDiv2
    li s11, NINV
# 345 layers, input is int32_t out[N], output is int32_t out[N]
looper_345:
    # s0-s7 = a[0,32,64,...,224]
    LoadS w, 32, 4, a1
    # level1
    BFUINTT s0, s1, t0, a5, a6, s8
    BFUINTT s2, s3, t1, a5, a6, s8
    BFUINTT s4, s5, t2, a5, a6, s8
    BFUINTT s6, s7, t3, a5, a6, s8
    # level2
    BFUINTT s0, s2, t4, a5, a6, s8
    BFUINTT s1, s3, t4, a5, a6, s8
    BFUINTT s4, s6, t5, a5, a6, s8
    BFUINTT s5, s7, t5, a5, a6, s8
    # level3
    BFUINTT s0, s4, t6, a5, a6, s8
    BFUINTT s1, s5, t6, a5, a6, s8
    BFUINTT s2, s6, t6, a5, a6, s8
    BFUINTT s3, s7, t6, a5, a6, s8
    # revert to standard domain and reduce to centered representation
    SCReduce s11, a5, a6, s10, s8, s9
    # save results
    StoreS 32, a1

    addi a1, a1, 4
    bne a1, a3, looper_345

    lw s0, 0*4(sp)
    lw s1, 1*4(sp)
    lw s2, 2*4(sp)
    lw s3, 3*4(sp)
    lw s4, 4*4(sp)
    lw s5, 5*4(sp)
    lw s6, 6*4(sp)
    lw s7, 7*4(sp)
    lw s8, 8*4(sp)
    lw s9, 9*4(sp)
    lw s10,10*4(sp)
    lw s11,11*4(sp)
    addi sp, sp, 4*12
    ret    

# void basemul_asm(int32_t a[SABER_N], const int32_t b[SABER_N], int32_t zeta[SABER_N/8])
# save results to a[SABER_N]
# register usage: t0-t2: parameters; s0-s7: a[0-7]; a0-a7: b[0-7]; 
# t3: zeta; t4: loop control; t5/t6: M/Mprime; s8-s11+gp+ra: temp;
.globl basemul_asm_5layer
.align 2
basemul_asm_5layer:
    addi sp, sp, -4*15
    sw s0, 0*4(sp)
    sw s1, 1*4(sp)
    sw s2, 2*4(sp)
    sw s3, 3*4(sp)
    sw s4, 4*4(sp)
    sw s5, 5*4(sp)
    sw s6, 6*4(sp)
    sw s7, 7*4(sp)
    sw s8, 8*4(sp)
    sw s9, 9*4(sp)
    sw s10,10*4(sp)
    sw s11,11*4(sp)
    sw gp, 12*4(sp)
    sw tp, 13*4(sp)
    sw ra, 14*4(sp)
    # parameters
    mv t0, a0
    mv t1, a1
    mv t2, a2
    # Load M, Mprime
    li t5, M
    li t6, Mprime
    # loop counter
    addi t4, t0, 32*8*4
base_looper:
    # a[0-7]
    lw s0, 0*4(t0)
    lw s1, 1*4(t0)
    lw s2, 2*4(t0)
    lw s3, 3*4(t0)
    lw s4, 4*4(t0)
    lw s5, 5*4(t0)
    lw s6, 6*4(t0)
    lw s7, 7*4(t0)
    # b[0-7]
    lw a0, 0*4(t1)
    lw a1, 1*4(t1)
    lw a2, 2*4(t1)
    lw a3, 3*4(t1)
    lw a4, 4*4(t1)
    lw a5, 5*4(t1)
    lw a6, 6*4(t1)
    lw a7, 7*4(t1)    
    # t3 = zeta
    lw t3, 0*4(t2)
    # r0=a0b0+zeta*(a1b7+a2b6+a3b5+a4b4+a5b3+a6b2+a7b1)
    MUL64 s1, a7, s8, s9
    MUL64 s2, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a1, s10, s11
    ADD64 s10, s11, s8, s9
    MontReduce s8, s9, s10, t5, t6
    # out: s8
    MontMulNew s10, t3, s8, t5, t6
    MontMul s0, a0, s9, t5, t6, s10
    add s8, s8, s9
    sw  s8, 0*4(t0)

    # r1=a0b1+a1b0+zeta*(a2b7+a3b6+a4b5+a5b4+a6b3+a7b2)
    MUL64 s2, a7, s8, s9
    MUL64 s3, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MontReduce s8, s9, s10, t5, t6
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a1, s8, s9
    MUL64 s1, a0, s10, s11
    ADD64 s10, s11, s8, s9
    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 1*4(t0)

    # r2=a0b2+a1b1+a2b0+zeta*(a3b7+a3b6+...+a7b3)
    MUL64 s3, a7, s8, s9
    MUL64 s4, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MontReduce s8, s9, s10, t5, t6
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a2, s8, s9
    MUL64 s1, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 2*4(t0)

    # r3=a0b3+a1b2+a2b1+a3b0+zeta*(a4b7+a5b6+...+a7b4)
    MUL64 s4, a7, s8, s9
    MUL64 s5, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MontReduce s8, s9, s10, t5, t6
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a3, s8, s9
    MUL64 s1, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 3*4(t0)

    # r4=a0b4+a1b3+a2b2+a3b1+a4b0+zeta*(a5b7+a6b6+a7b5)
    MUL64 s5, a7, s8, s9
    MUL64 s6, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MontReduce s8, s9, s10, t5, t6
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a4, s8, s9
    MUL64 s1, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 4*4(t0)

    # r5=a0b5+a1b4+a2b3+a3b2+a4b1+a5b0+zeta*(a6b7+a7b6)
    MUL64 s6, a7, s8, s9
    MUL64 s7, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MontReduce s8, s9, s10, t5, t6
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a5, s8, s9
    MUL64 s1, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 5*4(t0)

    # r6=a0b6+a1b5+a2b4+a3b3+a4b2+a5b1+a6b0+zeta*(a7b7)
    MontMul s7, a7, s10, t5, t6, s8
    # out: gp
    MontMulNew s10, t3, gp, t5, t6

    MUL64 s0, a6, s8, s9
    MUL64 s1, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    add s10, s10, gp
    sw  s10, 6*4(t0)

    # r7=a0b7+a1b6+a2b5+a3b4+a4b3+a5b2+a6b1+a7b0
    MUL64 s0, a7, s8, s9
    MUL64 s1, a6, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s2, a5, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s3, a4, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s4, a3, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s5, a2, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s6, a1, s10, s11
    ADD64 s10, s11, s8, s9

    MUL64 s7, a0, s10, s11
    ADD64 s10, s11, s8, s9

    # out: s10
    MontReduce s8, s9, s10, t5, t6
    sw  s10, 7*4(t0)

    # update index
    addi t0, t0, 8*4
    addi t1, t1, 8*4
    addi t2, t2, 4
    bne  t4, t0, base_looper

    lw s0, 0*4(sp)
    lw s1, 1*4(sp)
    lw s2, 2*4(sp)
    lw s3, 3*4(sp)
    lw s4, 4*4(sp)
    lw s5, 5*4(sp)
    lw s6, 6*4(sp)
    lw s7, 7*4(sp)
    lw s8, 8*4(sp)
    lw s9, 9*4(sp)
    lw s10,10*4(sp)
    lw s11,11*4(sp)
    lw gp, 12*4(sp)
    lw tp, 13*4(sp)
    lw ra, 14*4(sp)
    addi sp, sp, 4*15
    ret