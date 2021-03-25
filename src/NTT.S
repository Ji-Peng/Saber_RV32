.text

.equ Mprime, 0x6007ff
.equ M, 0xa00801
.equ V, 0x199851d

# int64_t a=in1*in2
# int32_t t=a*Mprime
# t=((int64_t)a+(int64_t)t*M)>>32
.macro MontMul in1, in2, out, tmp1, tmp2
    # out=low(in1*in2)=a_low
    mul \out, \in1, \in2
    # a_low=0: carry=0, a_low!=0: carry=1
    snez \tmp2, \out
    # tmp1=Mprime
    li \tmp1, Mprime
    # out=t=a_low*Mprime
    mul \out, \out, \tmp1
    # tmp1=M
    li \tmp1, M
    # out=high(t*M)
    mulh \out, \out, \tmp1
    # add carry
    add \out, \out, \tmp2
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
# in1, in2: t=zeta*in1, in2=in1-t; in1=in1+t;
.macro BFU in1, in2, zeta, tmp1, tmp2, tmp3
    # tmp1=zeta*in1
    MontMul \in1, \in2, \tmp1, \tmp2, \tmp3
    # in2=in1-t
    sub \in2, \in1, \tmp1
    # in1=in1+t
    add \in1, \in1, \tmp1
.endm

# .globl ntt_asm
# .align 2
# ntt_asm:
#     ret

# .globl FqMulAsm
# .align 2
# FqMulAsm:
#     MontMul a0, a1, a3, a4, a5
#     mv a0, a3
#     ret

# .globl BarrettReduceAsm
# .align 2
# BarrettReduceAsm:
#     BarrettReduce a0, a1, a2
#     mv a0, a1
#     ret