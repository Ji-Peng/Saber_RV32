.text

# M * Mprime = -1 mod R (R=2^32), M and Mprime are used for montgomery reduce
.equ Mprime, 0x6007ff
.equ M, 0xa00801
# v = (((int64_t)1 << 48) + M / 2) / M is used for barrett reduce
.equ V, 0x199851d
# NINV = R^2 * (1/256) mod M is used for reverting standard domain
.equ NINV, 0x316ad1

# int64_t a=in1*in2
# int32_t t=a*Mprime
# t=((int64_t)a+(int64_t)t*M)>>32
.macro MontMul in1, in2, out, CM, CMprime, tmp1
    # out=low(in1*in2)=a_low
    mul \out, \in1, \in2
    # a_low=0: carry=0, a_low!=0: carry=1
    snez \tmp1, \out
    # out=t=a_low*Mprime
    mul \out, \out, \CMprime
    # out=high(t*M)
    mulh \out, \out, \CM
    # add carry
    add \out, \out, \tmp1
    # high(in1*in2)
    mulh \tmp1, \in1, \in2
    add \out, \out, \tmp1
.endm

# int64_t a=inHi&&inLo
# inHi and inLo wille be rewrited
.macro MontReduce inLo, inHi, out, CM, CMPrime
    # t=out=low(a*Mprime)
    mul \out, \inLo, \CMPrime
    # a_low=0: carry=0; a_low!=0: carry=1
    snez \inLo, \inLo
    # out=high(t*M)
    mulh \out, \out, \CM
    # high(t*M)+a_high
    add \out, \out, \inHi
    # add carry
    add \out, \out, \inLo
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
.macro BFU in1, in2, zeta, CM, CMprime, tmp1, tmp2
    # tmp1=zeta*in1
    MontMul \in2, \zeta, \tmp1, \CM, \CMprime, \tmp2
    # in2=in1-t
    sub \in2, \in1, \tmp1
    # in1=in1+t
    add \in1, \in1, \tmp1
.endm

# Butterfly Unit for INTT
# in1, in2: t=in1; in1=t+in2; in2=t-in2; in2=zeta*in2
.macro BFUINTT in1, in2, zeta, CM, CMprime, tmp1, tmp2
    # t=in1
    mv \tmp1, \in1
    # in1=t+in2
    add \in1, \tmp1, \in2
    # t=t-in2
    sub \tmp1, \tmp1, \in2
    # in2=zeta*t
    MontMul \zeta, \tmp1, \in2, \CM, \CMprime, \tmp2
.endm

# revert to Standard domain and reduce to Centered representation
.macro SCReduce
    MontMul s0, tp, t4, t6, a7, t5
    BarrettReduce t4, s0, t5
    MontMul s1, tp, t4, t6, a7, t5
    BarrettReduce t4, s1, t5
    MontMul s2, tp, t4, t6, a7, t5
    BarrettReduce t4, s2, t5
    MontMul s3, tp, t4, t6, a7, t5
    BarrettReduce t4, s3, t5     

    MontMul s4, tp, t4, t6, a7, t5
    BarrettReduce t4, s4, t5
    MontMul s5, tp, t4, t6, a7, t5
    BarrettReduce t4, s5, t5
    MontMul s6, tp, t4, t6, a7, t5
    BarrettReduce t4, s6, t5
    MontMul s7, tp, t4, t6, a7, t5
    BarrettReduce t4, s7, t5 

    MontMul s8, tp, t4, t6, a7, t5
    BarrettReduce t4, s8, t5
    MontMul s9, tp, t4, t6, a7, t5
    BarrettReduce t4, s9, t5
    MontMul s10, tp, t4, t6, a7, t5
    BarrettReduce t4, s10, t5
    MontMul s11, tp, t4, t6, a7, t5
    BarrettReduce t4, s11, t5 

    MontMul a3, tp, t4, t6, a7, t5
    BarrettReduce t4, a3, t5
    MontMul a4, tp, t4, t6, a7, t5
    BarrettReduce t4, a4, t5
    MontMul a5, tp, t4, t6, a7, t5
    BarrettReduce t4, a5, t5
    MontMul a6, tp, t4, t6, a7, t5
    BarrettReduce t4, a6, t5                    
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
    l\hw s8, \len*\wordLen*8(\reg)
    l\hw s9, \len*\wordLen*9(\reg)
    l\hw s10, \len*\wordLen*10(\reg)
    l\hw s11, \len*\wordLen*11(\reg)
    l\hw a3, \len*\wordLen*12(\reg)
    l\hw a4, \len*\wordLen*13(\reg)
    l\hw a5, \len*\wordLen*14(\reg)
    l\hw a6, \len*\wordLen*15(\reg)
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
# const int32_t rootTable[256]);
.globl ntt_asm_8layer
.align 2
ntt_asm_8layer:
    addi sp, sp, -4*14
    sw s0,  0*4(sp)
    sw s1,  1*4(sp)
    sw s2,  2*4(sp)
    sw s3,  3*4(sp)
    sw s4,  4*4(sp)
    sw s5,  5*4(sp)
    sw s6,  6*4(sp)
    sw s7,  7*4(sp)
    sw s8,  8*4(sp)
    sw s9,  9*4(sp)
    sw s10, 10*4(sp)
    sw s11, 11*4(sp)
    sw gp,  12*4(sp)
    sw tp,  13*4(sp)

    # zeta, M, Mprime
    lw t0, 0*4(a2)
    lw t1, 1*4(a2)
    lw t2, 2*4(a2)
    lw t3, 3*4(a2)

    li t6, M
    li a7, Mprime
    # loop control, a0+32=a[16], 16: loop numbers, 2: uint16_t=2B
    addi gp, a0, 16*2
# 1234 layers, input is uint16_t in[N], output is int32_t out[N]
looper_1234:
    # a[0,16,32,...,240]
    LoadS h, 16, 2, a0
    # level1
    BFU s0, s8,  t0, t6, a7, t4, t5
    BFU s1, s9,  t0, t6, a7, t4, t5
    BFU s2, s10, t0, t6, a7, t4, t5
    BFU s3, s11, t0, t6, a7, t4, t5
    BFU s4, a3,  t0, t6, a7, t4, t5
    BFU s5, a4,  t0, t6, a7, t4, t5
    BFU s6, a5,  t0, t6, a7, t4, t5
    BFU s7, a6,  t0, t6, a7, t4, t5    
    # level2
    BFU s0,  s4, t1, t6, a7, t4, t5
    BFU s1,  s5, t1, t6, a7, t4, t5
    BFU s2,  s6, t1, t6, a7, t4, t5
    BFU s3,  s7, t1, t6, a7, t4, t5 
    BFU s8,  a3, t2, t6, a7, t4, t5
    BFU s9,  a4, t2, t6, a7, t4, t5
    BFU s10, a5, t2, t6, a7, t4, t5
    BFU s11, a6, t2, t6, a7, t4, t5     
    # level3
    BFU s0,  s2,  t3, t6, a7, t4, t5
    BFU s1,  s3,  t3, t6, a7, t4, t5
    lw  tp,  4*4(a2)
    BFU s4,  s6,  tp, t6, a7, t4, t5
    BFU s5,  s7,  tp, t6, a7, t4, t5 
    lw  tp,  5*4(a2)
    BFU s8,  s10, tp, t6, a7, t4, t5
    BFU s9,  s11, tp, t6, a7, t4, t5
    lw  tp,  6*4(a2)
    BFU a3,  a5,  tp, t6, a7, t4, t5
    BFU a4,  a6,  tp, t6, a7, t4, t5
    # level4
    lw  tp,  7*4(a2)
    BFU s0,  s1,  tp, t6, a7, t4, t5
    lw  tp,  8*4(a2)
    BFU s2,  s3,  tp, t6, a7, t4, t5
    lw  tp,  9*4(a2)
    BFU s4,  s5,  tp, t6, a7, t4, t5
    lw  tp,  10*4(a2)
    BFU s6,  s7,  tp, t6, a7, t4, t5
    lw  tp,  11*4(a2)
    BFU s8,  s9,  tp, t6, a7, t4, t5
    lw  tp,  12*4(a2)
    BFU s10, s11, tp, t6, a7, t4, t5
    lw  tp,  13*4(a2)
    BFU a3,  a4,  tp, t6, a7, t4, t5
    lw  tp,  14*4(a2)
    BFU a5,  a6,  tp, t6, a7, t4, t5  
    //save results
    StoreS 16, a1

    addi a0, a0, 2
    addi a1, a1, 4
    bne gp, a0, looper_1234

# ============== level 5678 ============== #
# reset a1, 16: inner loop number, 4: wordLen
addi a1, a1, -16*4
# a2 points to next 15 zetas
addi a2, a2, 4*15
# set outer loop control register gp, 16: block number in 5th layer
# and there are 16 coefficients in each block
addi gp, a2, 16*4*15

looper_5678:
    # a[0,1,...,15]
    LoadS w, 1, 4, a1
    lw  t0, 0*4(a2)
    lw  t1, 1*4(a2)
    lw  t2, 2*4(a2)
    lw  t3, 3*4(a2)
    # level5
    BFU s0, s8,  t0, t6, a7, t4, t5
    BFU s1, s9,  t0, t6, a7, t4, t5
    BFU s2, s10, t0, t6, a7, t4, t5
    BFU s3, s11, t0, t6, a7, t4, t5
    BFU s4, a3,  t0, t6, a7, t4, t5
    BFU s5, a4,  t0, t6, a7, t4, t5
    BFU s6, a5,  t0, t6, a7, t4, t5
    BFU s7, a6,  t0, t6, a7, t4, t5 
    # level6
    BFU s0,  s4,  t1, t6, a7, t4, t5
    BFU s1,  s5,  t1, t6, a7, t4, t5
    BFU s2,  s6,  t1, t6, a7, t4, t5
    BFU s3,  s7,  t1, t6, a7, t4, t5
    BFU s8,  a3,  t2, t6, a7, t4, t5
    BFU s9,  a4,  t2, t6, a7, t4, t5
    BFU s10, a5,  t2, t6, a7, t4, t5
    BFU s11, a6,  t2, t6, a7, t4, t5 
    # level7
    BFU s0,  s2,  t3, t6, a7, t4, t5
    BFU s1,  s3,  t3, t6, a7, t4, t5
    lw  t0, 4*4(a2)
    lw  t1, 5*4(a2)
    lw  t2, 6*4(a2)
    lw  t3, 7*4(a2)
    BFU s4,  s6,  t0, t6, a7, t4, t5
    BFU s5,  s7,  t0, t6, a7, t4, t5
    BFU s8,  s10, t1, t6, a7, t4, t5
    BFU s9,  s11, t1, t6, a7, t4, t5
    BFU a3,  a5,  t2, t6, a7, t4, t5
    BFU a4,  a6,  t2, t6, a7, t4, t5 
    # level8
    BFU s0,  s1,  t3, t6, a7, t4, t5
    lw  t0, 8*4(a2)
    lw  t1, 9*4(a2)
    lw  t2, 10*4(a2)
    lw  t3, 11*4(a2)
    BFU s2,  s3,  t0, t6, a7, t4, t5
    BFU s4,  s5,  t1, t6, a7, t4, t5
    BFU s6,  s7,  t2, t6, a7, t4, t5
    BFU s8,  s9,  t3, t6, a7, t4, t5
    lw  t0, 12*4(a2)
    lw  t1, 13*4(a2)
    lw  t2, 14*4(a2)
    BFU s10, s11, t0, t6, a7, t4, t5
    BFU a3,  a4,  t1, t6, a7, t4, t5
    BFU a5,  a6,  t2, t6, a7, t4, t5 
    //save results
    StoreS 1, a1
    # a1 points to next block, a2 points to next 7 zetas
    # each block has 16 words
    addi a1, a1, 16*4
    addi a2, a2, 4*15
    bne  gp, a2, looper_5678

    lw s0,  0*4(sp)
    lw s1,  1*4(sp)
    lw s2,  2*4(sp)
    lw s3,  3*4(sp)
    lw s4,  4*4(sp)
    lw s5,  5*4(sp)
    lw s6,  6*4(sp)
    lw s7,  7*4(sp)
    lw s8,  8*4(sp)
    lw s9,  9*4(sp)
    lw s10, 10*4(sp)
    lw s11, 11*4(sp)
    lw gp,  12*4(sp)
    lw tp,  13*4(sp)
    addi sp, sp, 4*14
    ret

# void intt_asm(const uint32_t in[256], int32_t out[256],
# const int32_t rootTable[256]);
.globl intt_asm_8layer
.align 2
intt_asm_8layer:
    addi sp, sp, -4*14
    sw s0,  0*4(sp)
    sw s1,  1*4(sp)
    sw s2,  2*4(sp)
    sw s3,  3*4(sp)
    sw s4,  4*4(sp)
    sw s5,  5*4(sp)
    sw s6,  6*4(sp)
    sw s7,  7*4(sp)
    sw s8,  8*4(sp)
    sw s9,  9*4(sp)
    sw s10, 10*4(sp)
    sw s11, 11*4(sp)
    sw gp,  12*4(sp)
    sw tp,  13*4(sp)

# set loop control register gp, 16: loop number
addi gp, a2, 16*4*15
li t6, M
li a7, Mprime
inv_looper_1234:
    # a[0,1,...,15]
    LoadS w, 1, 4, a0
    # level1
    lw  t0, 0*4(a2)
    lw  t1, 1*4(a2)
    lw  t2, 2*4(a2)
    lw  t3, 3*4(a2)
    BFUINTT s0, s1, t0, t6, a7, t4, t5
    BFUINTT s2, s3, t1, t6, a7, t4, t5
    BFUINTT s4, s5, t2, t6, a7, t4, t5
    BFUINTT s6, s7, t3, t6, a7, t4, t5
    lw  t0, 4*4(a2)
    lw  t1, 5*4(a2)
    lw  t2, 6*4(a2)
    lw  t3, 7*4(a2)
    BFUINTT s8,  s9,  t0, t6, a7, t4, t5
    BFUINTT s10, s11, t1, t6, a7, t4, t5
    BFUINTT a3,  a4,  t2, t6, a7, t4, t5
    BFUINTT a5,  a6,  t3, t6, a7, t4, t5    
    # level2
    lw  t0, 8*4(a2)
    lw  t1, 9*4(a2)
    lw  t2, 10*4(a2)
    lw  t3, 11*4(a2)
    BFUINTT s0, s2,  t0, t6, a7, t4, t5
    BFUINTT s1, s3,  t0, t6, a7, t4, t5
    BFUINTT s4, s6,  t1, t6, a7, t4, t5
    BFUINTT s5, s7,  t1, t6, a7, t4, t5
    BFUINTT s8, s10, t2, t6, a7, t4, t5
    BFUINTT s9, s11, t2, t6, a7, t4, t5
    BFUINTT a3, a5,  t3, t6, a7, t4, t5
    BFUINTT a4, a6,  t3, t6, a7, t4, t5          
    # level3
    lw  t0, 12*4(a2)
    lw  t1, 13*4(a2)
    BFUINTT s0,  s4,  t0, t6, a7, t4, t5
    BFUINTT s1,  s5,  t0, t6, a7, t4, t5
    BFUINTT s2,  s6,  t0, t6, a7, t4, t5
    BFUINTT s3,  s7,  t0, t6, a7, t4, t5
    BFUINTT s8,  a3,  t1, t6, a7, t4, t5
    BFUINTT s9,  a4,  t1, t6, a7, t4, t5
    BFUINTT s10, a5,  t1, t6, a7, t4, t5
    BFUINTT s11, a6,  t1, t6, a7, t4, t5
    # level4
    lw  t0, 14*4(a2)
    BFUINTT s0, s8,  t0, t6, a7, t4, t5
    BFUINTT s1, s9,  t0, t6, a7, t4, t5
    BFUINTT s2, s10, t0, t6, a7, t4, t5
    BFUINTT s3, s11, t0, t6, a7, t4, t5
    BFUINTT s4, a3,  t0, t6, a7, t4, t5
    BFUINTT s5, a4,  t0, t6, a7, t4, t5
    BFUINTT s6, a5,  t0, t6, a7, t4, t5
    BFUINTT s7, a6,  t0, t6, a7, t4, t5    
    //save results
    StoreS 1, a1
    # a0,a1 point to next block, a2 points to next 7 zetas
    # each block has 32 words, inner loop's increment is 4 words
    addi a0, a0, 16*4
    addi a1, a1, 16*4
    addi a2, a2, 4*15
    bne  a2, gp, inv_looper_1234

    //========layer5678========//
    # reset a1
    addi a1, a1, -16*16*4
    # loop control, 16: loop numbers, 4: int32_t=4B
    addi gp, a1, 16*4
    # Load NINV for reverting to standard domain
    li tp, NINV
    # load zetas
    lw  t0, 12*4(a2)
    lw  t1, 13*4(a2)
    lw  t2, 14*4(a2)
# 5678 layers, input is int32_t out[N], output is int32_t out[N]
inv_looper_5678:
    LoadS w, 16, 4, a1
    # level1
    lw  t3,  0*4(a2)
    BFUINTT s0,  s1,  t3, t6, a7, t4, t5
    lw  t3,  1*4(a2)
    BFUINTT s2,  s3,  t3, t6, a7, t4, t5
    lw  t3,  2*4(a2)
    BFUINTT s4,  s5,  t3, t6, a7, t4, t5
    lw  t3,  3*4(a2)
    BFUINTT s6,  s7,  t3, t6, a7, t4, t5
    lw  t3,  4*4(a2)
    BFUINTT s8,  s9,  t3, t6, a7, t4, t5
    lw  t3,  5*4(a2)
    BFUINTT s10, s11, t3, t6, a7, t4, t5
    lw  t3,  6*4(a2)
    BFUINTT a3,  a4,  t3, t6, a7, t4, t5
    lw  t3,  7*4(a2)
    BFUINTT a5,  a6,  t3, t6, a7, t4, t5  
    # level2
    lw  t3,  8*4(a2)
    BFUINTT s0,  s2,  t3, t6, a7, t4, t5
    BFUINTT s1,  s3,  t3, t6, a7, t4, t5
    lw  t3,  9*4(a2)
    BFUINTT s4,  s6,  t3, t6, a7, t4, t5
    BFUINTT s5,  s7,  t3, t6, a7, t4, t5 
    lw  t3,  10*4(a2)
    BFUINTT s8,  s10, t3, t6, a7, t4, t5
    BFUINTT s9,  s11, t3, t6, a7, t4, t5
    lw  t3,  11*4(a2)
    BFUINTT a3,  a5,  t3, t6, a7, t4, t5
    BFUINTT a4,  a6,  t3, t6, a7, t4, t5    
    # level3
    BFUINTT s0,  s4, t0, t6, a7, t4, t5
    BFUINTT s1,  s5, t0, t6, a7, t4, t5
    BFUINTT s2,  s6, t0, t6, a7, t4, t5
    BFUINTT s3,  s7, t0, t6, a7, t4, t5 
    BFUINTT s8,  a3, t1, t6, a7, t4, t5
    BFUINTT s9,  a4, t1, t6, a7, t4, t5
    BFUINTT s10, a5, t1, t6, a7, t4, t5
    BFUINTT s11, a6, t1, t6, a7, t4, t5      
    # level4
    BFUINTT s0, s8,  t2, t6, a7, t4, t5
    BFUINTT s1, s9,  t2, t6, a7, t4, t5
    BFUINTT s2, s10, t2, t6, a7, t4, t5
    BFUINTT s3, s11, t2, t6, a7, t4, t5
    BFUINTT s4, a3,  t2, t6, a7, t4, t5
    BFUINTT s5, a4,  t2, t6, a7, t4, t5
    BFUINTT s6, a5,  t2, t6, a7, t4, t5
    BFUINTT s7, a6,  t2, t6, a7, t4, t5     
    # revert to standard domain and reduce to centered representation
    SCReduce
    # save results
    StoreS 16, a1

    addi a1, a1, 4
    bne a1, gp, inv_looper_5678

    lw s0,  0*4(sp)
    lw s1,  1*4(sp)
    lw s2,  2*4(sp)
    lw s3,  3*4(sp)
    lw s4,  4*4(sp)
    lw s5,  5*4(sp)
    lw s6,  6*4(sp)
    lw s7,  7*4(sp)
    lw s8,  8*4(sp)
    lw s9,  9*4(sp)
    lw s10, 10*4(sp)
    lw s11, 11*4(sp)
    lw gp,  12*4(sp)
    lw tp,  13*4(sp)
    addi sp, sp, 4*14
    ret    

# void basemul_asm_8layer(int32_t a[256], const int32_t b[256])
# in-place implementation a[i]=a[i]*b[i]
.globl basemul_asm_8layer
.align 2
basemul_asm_8layer:
    li t4, M
    li t5, Mprime
    # looper control, each loop processes 4 elements, loop number is 64
    addi t6, a0, 64*4*4
base_looper:
    # t0-t3=a[0]-a[3]
    lw t0, 0*4(a0)
    lw t1, 1*4(a0)
    lw t2, 2*4(a0)
    lw t3, 3*4(a0)
    # a4-a7=b[0]-b[3]
    lw a4, 0*4(a1)
    lw a5, 1*4(a1)
    lw a6, 2*4(a1)
    lw a7, 3*4(a1)
    # a[0]*b[0]-a[3]*b[3]
    MontMul t0, a4, a2, t4, t5, a3
    MontMul t1, a5, t0, t4, t5, a3
    MontMul t2, a6, t1, t4, t5, a3
    MontMul t3, a7, t2, t4, t5, a3
    # save results
    sw a2, 0*4(a0)
    sw t0, 1*4(a0)
    sw t1, 2*4(a0)
    sw t2, 3*4(a0)
    # update index
    addi a0, a0, 4*4
    addi a1, a1, 4*4
    bne t6, a0, base_looper
    ret