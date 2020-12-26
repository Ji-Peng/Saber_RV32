# The configuration defaults to Debug. Valid choices are: debug and release
CONFIGURATION ?= debug
# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR = $(abspath bsp)
# SRC_DIR sets the path to the program source directory
SRC_DIR = src
COMMON_DIR = benchmark/common

# Allow users to select a different cross compiler.
CROSS_COMPILE ?= riscv64-unknown-elf
RISCV_GCC     := $(CROSS_COMPILE)-gcc
RISCV_OBJDUMP := $(CROSS_COMPILE)-objdump
RISCV_OBJCOPY := $(CROSS_COMPILE)-objcopy
RISCV_GDB     := $(CROSS_COMPILE)-gdb
RISCV_AR      := $(CROSS_COMPILE)-ar
RISCV_SIZE    := $(CROSS_COMPILE)-size

ARCH_FLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medlow
SPEC=nano
MTIME_RATE_HZ_DEF=32768
RISCV_CFLAGS  	+= 	-O0 -g \
					-Wall -Wextra -Wimplicit-function-declaration \
              		-Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
              		-Wundef -Wshadow \
					-I$(abspath $(BSP_DIR)/install/include/) \
					-fno-common -ffunction-sections -fdata-sections --specs=$(SPEC).specs \
					-DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF) \
					$(ARCH_FLAGS)

RISCV_LDFLAGS 	+= 	-Wl,--start-group  -lc -lgcc -lm -lmetal -lmetal-gloss -Wl,--end-group \
					-Wl,-Map,$(basename $@).map \
					-T$(abspath $(filter %.lds,$^)) -Xlinker --defsym=__heap_max=0x1 \
					 -nostartfiles -nostdlib -Wl,--gc-sections \
					$(ARCH_FLAGS) \
					-L$(sort $(dir $(abspath $(filter %.a,$^))))

.PHONY: all
all: out/kem.elf out/PQCgenKAT_kem.elf out/test_kex.elf

PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)
COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c) $(wildcard $(COMMON_DIR)/*.S)

out/%.elf: \
		benchmark/%.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS) \
		benchmark/common/libmetal.a \
		benchmark/common/libmetal-gloss.a \
		benchmark/common/metal.default.lds
	mkdir -p $(dir $@)
	$(RISCV_GCC) -o $(basename $@) $(RISCV_CFLAGS) \
		$(filter %.c,$^) $(filter %.S,$^) -I$(COMMON_DIR) -I$(SRC_DIR) $(RISCV_LDFLAGS)
	mv $(basename $@) $@
	touch -c $@
	$(RISCV_OBJDUMP) --source --all-headers --demangle --line-numbers --wide $@ > $(basename $@).lst
	$(RISCV_SIZE) $@
	$(RISCV_OBJCOPY) -O ihex $@ $(basename $@).hex

.PHONY: clean-software
clean-software:
	rm -rf out

.PHONY: clean
clean: clean-software