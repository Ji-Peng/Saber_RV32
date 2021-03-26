.text

.equ Mprime, 0x6007ff
.equ M, 0xa00801
.equ V, 0x199851d

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

# Butterfly Unit
# in1, in2: t=zeta*in2, in2=in1-t; in1=in1+t;
.macro BFU in1, in2, zeta, CM, CMprime, tmp1, tmp2
    # tmp1=zeta*in1
    MontMul \in2, \zeta, \tmp1, \CM, \CMprime, \tmp2
    # in2=in1-t
    sub \in2, \in1, \tmp1
    # in1=in1+t
    add \in1, \in1, \tmp1
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

# void ntt_asm(const uint16_t in[256], int32_t out[256],
# const int32_t rootTable[64]);
# register usage: a0-a2: parameter, t0-t7: zetas, s0-s7: 8 coefficients
# s8, s9 temporary usage, a6/a7: M/Mprime, a4/a5: looper control
.globl ntt_asm
.align 2
ntt_asm:
    addi sp, sp, -4*10
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

    # zeta, M, Mprime
    LoadZetas
    li a6, M
    li a7, Mprime
    # loop control, a0+64=a[32], 32: loop numbers, 2: uint16_t=2B
    addi a4, a0, 32*2
# 123 layers, input is uint16_t in[N], output is int32_t out[N]
looper_123:
    # s0-s7 = a[0,32,64,...,224]
    LoadS h, 32, 2, a0
    # level1
    BFU s0, s4, t0, a6, a7, s8, s9
    BFU s1, s5, t0, a6, a7, s8, s9
    BFU s2, s6, t0, a6, a7, s8, s9
    BFU s3, s7, t0, a6, a7, s8, s9
    # level2
    BFU s0, s2, t1, a6, a7, s8, s9
    BFU s1, s3, t1, a6, a7, s8, s9
    BFU s4, s6, t2, a6, a7, s8, s9
    BFU s5, s7, t2, a6, a7, s8, s9
    # level3
    BFU s0, s1, t3, a6, a7, s8, s9
    BFU s2, s3, t4, a6, a7, s8, s9
    BFU s4, s5, t5, a6, a7, s8, s9
    BFU s6, s7, t6, a6, a7, s8, s9
    //save results
    StoreS 32, a1

    addi a0, a0, 2
    addi a1, a1, 4
    bne a4, a0, looper_123

# ============== level 4-6 ============== #
# reset a1, 32: inner loop number, 4: wordLen
addi a1, a1, -32*4
# a2 points to next 7 zetas
addi a2, a2, 4*7
# set outer loop control register a5, 8: inner loop number
addi a5, a2, 8*4*7

outer_456:
    LoadZetas
    # set looper control register a4, inner loop's increment is 4 words
    addi a4, a1, 4*4

    inner_456:
        # a[0,4,...,28]
        LoadS w, 4, 4, a1
        # level1
        BFU s0, s4, t0, a6, a7, s8, s9
        BFU s1, s5, t0, a6, a7, s8, s9
        BFU s2, s6, t0, a6, a7, s8, s9
        BFU s3, s7, t0, a6, a7, s8, s9
        # level2
        BFU s0, s2, t1, a6, a7, s8, s9
        BFU s1, s3, t1, a6, a7, s8, s9
        BFU s4, s6, t2, a6, a7, s8, s9
        BFU s5, s7, t2, a6, a7, s8, s9
        # level3
        BFU s0, s1, t3, a6, a7, s8, s9
        BFU s2, s3, t4, a6, a7, s8, s9
        BFU s4, s5, t5, a6, a7, s8, s9
        BFU s6, s7, t6, a6, a7, s8, s9

        //save results
        StoreS 4, a1
        # looper control register changes
        addi a1, a1, 4
        bne a4, a1, inner_456
    # a1 points to next block, a2 points to next 7 zetas
    # each block has 32 words, inner loop's increment is 4 words
    addi a1, a1, 32*4-4*4
    addi a2, a2, 4*7
    bne  a5, a2, outer_456
    // outer loop control register


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
    addi sp, sp, 4*10
    ret

.size ntt_asm,.-ntt_asm