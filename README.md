# Saber on RV32IMAC

## SiFive Platform
### Experimental Setup

Development Board: [SiFive FE310 RISC-V CPU](https://www.sifive.com/boards/hifive1-rev-b)

Project:

* `benchmark/`: benchmark file, including libmetal-gloss.a and link script metal.default.lds
* `bsp/`: board support package
* `help/`: ipython script for searching parameters and c file for generating tables used in ntt.
* `src/`: source file
* `.clang-format`: format script for vscode
* `gdb_script`: custom functions for gdb
* `jlink.sh`: jlink script for upload and debug
* `listen.py`: python script for listening serial port

### Upload and Debug

```bash
./jlink.sh --hex out/bench.hex --jlink JLinkExe
./jlink.sh --elf out/bench.elf --jlink JLinkGDBServer --gdb riscv64-unknown-elf-gdb
```

For the usage of GDBServer, we refer to [wiki](https://wiki.segger.com/J-Link_GDB_Server).

## PQRISCV Platform

About the experimental setup, we refer to [Kyber_RISC_V_Thesis](https://github.com/denigreco/Kyber_RISC_V_Thesis), [PQRISCV](https://github.com/mupq/pqriscv), and [PQRISCV-VEXRISCV](https://github.com/mupq/pqriscv-vexriscv).