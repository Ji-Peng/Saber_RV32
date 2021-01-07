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

## 实验结果

### -O3选项下

stack size = 0x2600（设为0x2700会导致堆空间无法使用）

运行kem.c测试，发现keypair可跑通，enc可跑通，dec栈溢出

stack size = 0x2600运行test_kex.c发现输出不完整，基本判定是堆空间不够用了，所以只能调整下栈空间

调整为0x2500后发现，堆空间又不足了

输出不完整并非是堆空间不足，而是nano lib没有实现printf("%llu")，所以需要将llu转换为string进行输出

调整为0x2560后，并且将llu转换为string，成功得到输出：

kp's cpucycles: 40115695
enc's cpucycles: 39824461
dec stack overflow
但是多次运行发现，每次运行的时间都不同，也就是说是非constant-time的啊？

结论：
1. stack size=0x2600可运行keypair和enc，dec栈溢出
2. 目前cpu cycles已测，但是发现并非是constant-time，github上开源的代码也找不到了，估计被他们删掉了吧～
3. 目前code size还每测，需要修改下测试脚本，不着急，很简单