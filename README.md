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

结论：
1. stack size=0x2600可运行keypair和enc，dec栈溢出
2. 目前cpu cycles还没测，因为当前的cpucycles实现错误
3. 目前code size还每测，需要修改下测试脚本，不着急，很简单