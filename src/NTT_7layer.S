.text

# M * Mprime = 1 mod R (R=2^32), M and Mprime are used for montgomery reduce
.equ Mprime, 0xff9ff801
.equ M, 0xa00801
# v = (((int64_t)1 << 48) + M / 2) / M is used for barrett reduce
.equ V, 0x199851d
# NINV = R^2 * (1/128) mod M is used for reverting standard domain
.equ NINV, 0x62d5a2
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
# inLo will be rewrited
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
.macro SCReduce CNINV, CMDiv2, CM, CMprime, tmp1, tmp2
    MontMulNew s0, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s0, \CM, \CMDiv2, \tmp2
    MontMulNew s1, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s1, \CM, \CMDiv2, \tmp2
    MontMulNew s2, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s2, \CM, \CMDiv2, \tmp2
    MontMulNew s3, \CNINV, \tmp1, \CM, \CMprime
    CenReduce \tmp1, s3, \CM, \CMDiv2, \tmp2

    CenReduce s4, s4, \CM, \CMDiv2, \tmp2
    CenReduce s5, s5, \CM, \CMDiv2, \tmp2
    CenReduce s6, s6, \CM, \CMDiv2, \tmp2
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

.macro LoadS_3layer hw, len, wordLen, reg
    l\hw s0, \len*\wordLen*0(\reg)
    l\hw s1, \len*\wordLen*1(\reg)
    l\hw s2, \len*\wordLen*2(\reg)
    l\hw s3, \len*\wordLen*3(\reg)
    l\hw s4, \len*\wordLen*4(\reg)
    l\hw s5, \len*\wordLen*5(\reg)
    l\hw s6, \len*\wordLen*6(\reg)
    l\hw s7, \len*\wordLen*7(\reg)
.endm

.macro StoreS_3layer len, reg
    sw s0, \len*4*0(\reg)
    sw s1, \len*4*1(\reg)
    sw s2, \len*4*2(\reg)
    sw s3, \len*4*3(\reg)
    sw s4, \len*4*4(\reg)
    sw s5, \len*4*5(\reg)
    sw s6, \len*4*6(\reg)
    sw s7, \len*4*7(\reg)
.endm

.macro LoadS_4layer hw, len, wordLen, reg
    l\hw s0, \len*\wordLen*0(\reg)
    l\hw s1, \len*\wordLen*1(\reg)
    l\hw s2, \len*\wordLen*2(\reg)
    l\hw s3, \len*\wordLen*3(\reg)
    l\hw s4, \len*\wordLen*4(\reg)
    l\hw s5, \len*\wordLen*5(\reg)
    l\hw s6, \len*\wordLen*6(\reg)
    l\hw s7, \len*\wordLen*7(\reg)
    l\hw s8, \len*\wordLen*8(\reg)
    l\hw s9, \len*\wordLen*9(\reg)
    l\hw s10, \len*\wordLen*10(\reg)
    l\hw s11, \len*\wordLen*11(\reg)
    l\hw a3, \len*\wordLen*12(\reg)
    l\hw a4, \len*\wordLen*13(\reg)
    l\hw a5, \len*\wordLen*14(\reg)
    l\hw a6, \len*\wordLen*15(\reg)
.endm

.macro StoreS_4layer len, reg
    sw s0, \len*4*0(\reg)
    sw s1, \len*4*1(\reg)
    sw s2, \len*4*2(\reg)
    sw s3, \len*4*3(\reg)
    sw s4, \len*4*4(\reg)
    sw s5, \len*4*5(\reg)
    sw s6, \len*4*6(\reg)
    sw s7, \len*4*7(\reg)
    sw s8, \len*4*8(\reg)
    sw s9, \len*4*9(\reg)
    sw s10, \len*4*10(\reg)
    sw s11, \len*4*11(\reg)
    sw a3, \len*4*12(\reg)
    sw a4, \len*4*13(\reg)
    sw a5, \len*4*14(\reg)
    sw a6, \len*4*15(\reg)
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
# const int32_t rootTable[64]);

# register usage for 3-layer merging: 
# a0-a2: parameter, s0-s7: 8 coefficients, t0-t6: zetas
# s8/s9: temporary usage, gp/ra: M/Mprime, s10/s11: looper control

# register usage for 4-layer merging:
# a0-a2: parameter, s0-s11+a3-a6: 16 coefficients
# t5: temporary usage, gp/ra: M/Mprime, t6/a7: looper control
# t0-t4+tp: zetas

# 3-layer merging + 4-layer merging
.globl ntt_asm_7layer
.align 2
ntt_asm_7layer:
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

    # zeta, M, Mprime
    LoadZetas
    li gp, M
    li ra, Mprime
    # loop control, loop counter is 256/8=32, 2: uint16_t=2B
    addi s10, a0, 32*2
# 123 layers, input is uint16_t in[N], output is int32_t out[N]
looper_123:
    # s0-s7 = a[0,32,64,...,224]
    LoadS_3layer h, 32, 2, a0
    # level1
    BFU s0, s4, t0, gp, ra, s8
    BFU s1, s5, t0, gp, ra, s8
    BFU s2, s6, t0, gp, ra, s8
    BFU s3, s7, t0, gp, ra, s8
    # level2
    BFU s0, s2, t1, gp, ra, s8
    BFU s1, s3, t1, gp, ra, s8
    BFU s4, s6, t2, gp, ra, s8
    BFU s5, s7, t2, gp, ra, s8
    # level3
    BFU s0, s1, t3, gp, ra, s8
    BFU s2, s3, t4, gp, ra, s8
    BFU s4, s5, t5, gp, ra, s8
    BFU s6, s7, t6, gp, ra, s8
    //save results
    StoreS_3layer 32, a1

    addi a0, a0, 2
    addi a1, a1, 4
    bne s10, a0, looper_123

# ============== level 4-7 ============== #
# reset a1, 32: inner loop number, 4: wordLen
addi a1, a1, -32*4
# a2 points to next 7 zetas
addi a2, a2, 4*7
# set outer loop control register t6, loop counter is 8, each loop uses 15 zetas
addi t6, a2, 8*4*15

outer_4567:
    # set looper control register a7, inner loop count is 2, each block has 2*16 coefficients
    addi a7, a1, 2*4

    lw  t0, 0*4(a2)
    lw  t1, 1*4(a2)
    lw  t2, 2*4(a2)
    lw  t3, 3*4(a2)
    lw  t4, 4*4(a2)
    inner_4567:
        # a[0,2,...,30]
        LoadS_4layer w, 2, 4, a1
        # level 4
        BFU s0, s8,  t0, gp, ra, t5
        BFU s1, s9,  t0, gp, ra, t5
        BFU s2, s10, t0, gp, ra, t5
        BFU s3, s11, t0, gp, ra, t5
        BFU s4, a3,  t0, gp, ra, t5
        BFU s5, a4,  t0, gp, ra, t5
        BFU s6, a5,  t0, gp, ra, t5
        BFU s7, a6,  t0, gp, ra, t5
        # level 5
        BFU s0,  s4,  t1, gp, ra, t5
        BFU s1,  s5,  t1, gp, ra, t5
        BFU s2,  s6,  t1, gp, ra, t5
        BFU s3,  s7,  t1, gp, ra, t5
        BFU s8,  a3,  t2, gp, ra, t5
        BFU s9,  a4,  t2, gp, ra, t5
        BFU s10, a5,  t2, gp, ra, t5
        BFU s11, a6,  t2, gp, ra, t5
        # level 6
        BFU s0,  s2,  t3, gp, ra, t5
        BFU s1,  s3,  t3, gp, ra, t5
        BFU s4,  s6,  t4, gp, ra, t5
        BFU s5,  s7,  t4, gp, ra, t5 
        lw  tp,  5*4(a2)
        BFU s8,  s10, tp, gp, ra, t5
        BFU s9,  s11, tp, gp, ra, t5
        lw  tp,  6*4(a2)
        BFU a3,  a5,  tp, gp, ra, t5
        BFU a4,  a6,  tp, gp, ra, t5
        # level 7
        lw  tp,  7*4(a2)
        BFU s0,  s1,  tp, gp, ra, t5
        lw  tp,  8*4(a2)
        BFU s2,  s3,  tp, gp, ra, t5
        lw  tp,  9*4(a2)
        BFU s4,  s5,  tp, gp, ra, t5
        lw  tp,  10*4(a2)
        BFU s6,  s7,  tp, gp, ra, t5
        lw  tp,  11*4(a2)
        BFU s8,  s9,  tp, gp, ra, t5
        lw  tp,  12*4(a2)
        BFU s10, s11, tp, gp, ra, t5
        lw  tp,  13*4(a2)
        BFU a3,  a4,  tp, gp, ra, t5
        lw  tp,  14*4(a2)
        BFU a5,  a6,  tp, gp, ra, t5
        //save results
        StoreS_4layer 2, a1
        # looper control register changes
        addi a1, a1, 4
        bne a7, a1, inner_4567
    # a1 points to next block, a2 points to next 15 zetas
    # each block has 32 words, inner loop's increment is 4 words
    addi a1, a1, 32*4-2*4
    addi a2, a2, 4*15
    bne  t6, a2, outer_4567

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

# void intt_asm(const uint32_t in[256], int32_t out[256],
# const int32_t rootTable[64]);

# register usage for 4-layer merging:
# a0-a2: parameter, s0-s11+a3-a6: 16 coefficients
# t5: temporary usage, gp/ra: M/Mprime, t6/a7: looper control
# t0-t4+tp: zetas

# register usage for 3-layer merging: 
# a0-a2: parameter, s0-s7: 8 coefficients, t0-t6: zetas
# s8/s9: NINV/MDiv2, gp/ra: M/Mprime, a3: looper control
# s10/s11: temp

# 4-layer merging + 3-layer merging
.globl intt_asm_7layer
.align 2
intt_asm_7layer:
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

li gp, M
li ra, Mprime
# set outer loop control register t6, loop counter is 8, each loop uses 15 zetas
addi t6, a2, 8*4*15
outer_1234:
    # set looper control register a4, inner loop's increment is 4 words, counter is 2
    addi a7, a1, 2*4

    lw  t0, 0*4(a2)
    lw  t1, 1*4(a2)
    lw  t2, 2*4(a2)
    lw  t3, 3*4(a2)
    lw  t4, 4*4(a2)

    inner_1234:
        # a[0,2,...,30]
        LoadS_4layer w, 2, 4, a0
        # level1
        BFUINTT s0, s1, t0, gp, ra, t5
        BFUINTT s2, s3, t1, gp, ra, t5
        BFUINTT s4, s5, t2, gp, ra, t5
        BFUINTT s6, s7, t3, gp, ra, t5
        BFUINTT s8, s9, t4, gp, ra, t5
        lw tp, 5*4(a2)
        BFUINTT s10,s11,tp, gp, ra, t5
        lw tp, 6*4(a2)
        BFUINTT a3, a4, tp, gp, ra, t5
        lw tp, 7*4(a2)
        BFUINTT a5, a6, tp, gp, ra, t5

        # level2
        lw  tp, 8*4(a2)
        BFUINTT s0, s2,  tp, gp, ra, t5
        BFUINTT s1, s3,  tp, gp, ra, t5
        lw  tp, 9*4(a2)
        BFUINTT s4, s6,  tp, gp, ra, t5
        BFUINTT s5, s7,  tp, gp, ra, t5
        lw  tp, 10*4(a2)
        BFUINTT s8, s10, tp, gp, ra, t5
        BFUINTT s9, s11, tp, gp, ra, t5
        lw  tp, 11*4(a2)
        BFUINTT a3, a5,  tp, gp, ra, t5
        BFUINTT a4, a6,  tp, gp, ra, t5

        # level3
        lw  tp, 12*4(a2)
        BFUINTT s0,  s4,  tp, gp, ra, t5
        BFUINTT s1,  s5,  tp, gp, ra, t5
        BFUINTT s2,  s6,  tp, gp, ra, t5
        BFUINTT s3,  s7,  tp, gp, ra, t5
        lw  tp, 13*4(a2)
        BFUINTT s8,  a3,  tp, gp, ra, t5
        BFUINTT s9,  a4,  tp, gp, ra, t5
        BFUINTT s10, a5,  tp, gp, ra, t5
        BFUINTT s11, a6,  tp, gp, ra, t5

        # level 4
        lw  tp, 14*4(a2)
        BFUINTT s0, s8,  tp, gp, ra, t5
        BFUINTT s1, s9,  tp, gp, ra, t5
        BFUINTT s2, s10, tp, gp, ra, t5
        BFUINTT s3, s11, tp, gp, ra, t5
        BFUINTT s4, a3,  tp, gp, ra, t5
        BFUINTT s5, a4,  tp, gp, ra, t5
        BFUINTT s6, a5,  tp, gp, ra, t5
        BFUINTT s7, a6,  tp, gp, ra, t5
        //save results
        StoreS_4layer 2, a1
        # looper control register changes
        addi a0, a0, 4
        addi a1, a1, 4
        bne  a1, a7, inner_1234
    # a0,a1 points to next block, a2 points to next 15 zetas
    # each block has 32 words, inner loop's increment is 4 words
    addi a0, a0, 32*4-2*4
    addi a1, a1, 32*4-2*4
    addi a2, a2, 4*15
    bne  a2, t6, outer_1234

    # reset a1
    addi a1, a1, -8*32*4
    # loop control, 32: loop numbers, 4: int32_t=4B
    addi a3, a1, 32*4
    LoadZetas
    # Load NINV for reverting to standard domain
    li s8, NINV
    li s9, MDiv2
# 567 layers, input is int32_t out[N], output is int32_t out[N]
looper_567:
    # s0-s7 = a[0,32,64,...,224]
    LoadS_3layer w, 32, 4, a1
    # level 5
    BFUINTT s0, s1, t0, gp, ra, s10
    BFUINTT s2, s3, t1, gp, ra, s10
    BFUINTT s4, s5, t2, gp, ra, s10
    BFUINTT s6, s7, t3, gp, ra, s10
    # level 6
    BFUINTT s0, s2, t4, gp, ra, s10
    BFUINTT s1, s3, t4, gp, ra, s10
    BFUINTT s4, s6, t5, gp, ra, s10
    BFUINTT s5, s7, t5, gp, ra, s10
    # level 7
    BFUINTT s0, s4, t6, gp, ra, s10
    BFUINTT s1, s5, t6, gp, ra, s10
    BFUINTT s2, s6, t6, gp, ra, s10
    BFUINTT s3, s7, t6, gp, ra, s10
    # revert to standard domain and reduce to centered representation
    SCReduce s8, s9, gp, ra, s10, s11
    # save results
    StoreS_3layer 32, a1

    addi a1, a1, 4
    bne a1, a3, looper_567

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

# void basemul_asm(int32_t a[N], const int32_t b[N], int32_t zeta[N/2])
# save results to a[N]
# register usage: a0,a1,a2 parameters; a3-a4: a[0]-a[1], a5-a6: b[0]-b[1], a7: zetas
# t0, t1: M/Mprime; t2-t5: temp, t6: loop control
.globl basemul_asm_7layer
.align 2
basemul_asm_7layer:
    # Load M, Mprime
    li t0, M
    li t1, Mprime
    # each loop processes 2 elements, loop number is 128
    addi t6, a0, 128*2*4
base_looper:    
    # load input
    lw a3, 0*4(a0) # a0
    lw a4, 1*4(a0) # a1
    lw a5, 0*4(a1) # b0
    lw a6, 1*4(a1) # b1
    lw a7, 0*4(a2) # zeta

    # r0=a0b0+zeta*(a1b1)
    # t2=FqMul(a0,b0)
    MontMul a3, a5, t2, t0, t1, t5
    # t3=FqMul(a1,b1)
    MontMul a4, a6, t3, t0, t1, t5
    # t4=FqMul(zeta,t3)
    MontMul a7, t3, t4, t0, t1, t5
    # t2+t4
    add t2, t2, t4
    sw  t2, 0*4(a0)

    # r1=a0b1+a1b0
    # t3||t2=a0*b1
    MUL64 a3, a6, t2, t3
    # t5||t4=a1*b0
    MUL64 a4, a5, t4, t5
    # a0b1+a1b0
    ADD64 t2, t3, t4, t5
    # MontReduce(t5||t4)
    MontReduce t4, t5, t2, t0, t1
    sw t2, 1*4(a0)

    # update index
    addi a0, a0, 2*4
    addi a1, a1, 2*4
    addi a2, a2, 1*4
    bne t6, a0, base_looper
    ret