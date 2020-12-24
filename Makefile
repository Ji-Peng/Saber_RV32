# The configuration defaults to Debug. Valid choices are: debug and release
CONFIGURATION ?= debug
# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR ?= $(abspath bsp)
# SRC_DIR sets the path to the program source directory
SRC_DIR ?= $(abspath src)

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
					-ffunction-sections -fdata-sections \
					-I$(abspath $(BSP_DIR)/install/include/) \
					--specs=$(SPEC).specs \
					-DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF) \
					$(ARCH_FLAGS)

RISCV_LDFLAGS 	+= 	-Wl,--start-group  -lc -lgcc -lm -lmetal -lmetal-gloss -Wl,--end-group \
					-Wl,-Map,$(basename $@).map \
					-T$(abspath $(filter %.lds,$^)) -nostartfiles -nostdlib -Wl,--gc-sections \
					$(ARCH_FLAGS) \
					-L$(sort $(dir $(abspath $(filter %.a,$^))))

.PHONY: all
all: out/bench.elf out/bench2.elf

PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)

out/%.elf: \
		benchmark/%.c $(PROGRAM_SRCS) \
		benchmark/libmetal.a \
		benchmark/libmetal-gloss.a \
		benchmark/metal.default.lds
	mkdir -p $(dir $@)
	$(RISCV_GCC) -o $(basename $@) $(RISCV_CFLAGS) \
		$< $(PROGRAM_SRCS) -I$(SRC_DIR) $(RISCV_LDFLAGS)
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