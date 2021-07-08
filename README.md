# Saber on RV32IMAC

## Different Trade-offs and in-complete NTT 

We use different MACRO to choose different time-memory trade-offs and different in-complete NTTs. See `src/SABER_params.h` for detailed information.

**Main MACRO used in our source code are as follows:**

* `SABER_L`: used to choose different variants of Saber. L=2 for LightSaber, L=3 for Saber, and L=4 for FireSaber.
* `FASTGENA_SLOWMUL`: strategy 1 in our paper
* `FASTGENA_FASTMUL`: strategy 2 in our paper
* `SLOWGENA_FASTMUL`: strategy 3 in our paper
* `FIVE_LAYER_NTT`: 5-layer incomplete-NTT
* `SIX_LAYER_NTT`: 6-layer incomplete-NTT
* `SEVEN_LAYER_NTT`: 7-layer incomplete-NTT
* `COMPLETE_NTT`: 8-layer complete-NTT
* `NTTASM`: define this MACRO will enable our assembly-NTT, otherwise the C-NTT is enabled

## Platforms
### SiFive Platform

#### Setup

Development Board: [SiFive FE310 RISC-V CPU](https://www.sifive.com/boards/hifive1-rev-b)

Project:

* `benchmark/`: benchmark related files
* `bsp/`: board support package
* `help/`: ipython script for searching parameters and c files for generating tables used in NTT.
* `src/`: source files
* `.clang-format`: format script for vscode
* `jlink.sh`: jlink script for upload and debug
* `listen.py`: python script for listening serial port

#### How to Run?

> Attention: Due to the limited memory resources of SiFive platform, you should slightly modify our Makefile when you want to test different metrics (speed or memory) for different variants of Saber (L=2, 3, or 4). The main parameter needed to be modified is "__stack_size", and we give detailed comments about how to choose the value.

```bash
# compile
make
# upload executable file to board:
./jlink.sh --hex out/kem.hex --jlink JLinkExe
# debug using GDBServer
./jlink.sh --elf out/kem.elf --jlink JLinkGDBServer --gdb riscv64-unknown-elf-gdb
```

For the usage of GDBServer, we refer the readers to [wiki](https://wiki.segger.com/J-Link_GDB_Server).

If you want to test stack usage, modify the Makefile from
```
.PHONY: all
all: out/kem.elf
```
to 

```
.PHONY: all
all: out/stack.elf
```
And the upload and debug commands become:
```bash
# compile
make
# upload executable file to board:
./jlink.sh --hex out/stack.hex --jlink JLinkExe
# debug using GDBServer
./jlink.sh --elf out/stack.elf --jlink JLinkGDBServer --gdb riscv64-unknown-elf-gdb
```

### PQRISCV Platform

About the experimental setup, we refer to [Kyber_RISC_V_Thesis](https://github.com/denigreco/Kyber_RISC_V_Thesis), [PQRISCV](https://github.com/mupq/pqriscv), and [PQRISCV-VEXRISCV](https://github.com/mupq/pqriscv-vexriscv).

```bash
sbt "runMain mupq.PQVexRiscvSim --init ../Saber_RV32/out/kem_vexrv.bin"
```
## Others

### Code Size

```bash
riscv64-unknown-elf-nm out/kem.elf --print-size --size-sort --radix=d | \
grep -v '\<_\|\<metal\|\<pll_configs' | \
awk '{sum+=$2 ; print $0} END{print "Total size =", sum, "bytes =", sum/1024, "kB"}'
```