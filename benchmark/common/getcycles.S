.text

.globl cpucycles
.align 2
cpucycles:
    csrr a1, mcycleh
    csrr a0, mcycle
    csrr a2, mcycleh
    bne a1, a2, cpucycles
    ret
.size cpucycles,.-cpucycles