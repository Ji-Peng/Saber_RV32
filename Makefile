# The configuration defaults to Debug. Valid choices are: debug and release
CONFIGURATION ?= debug
# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR = $(abspath bsp)
# SRC_DIR sets the path to the program source directory
SRC_DIR = src
COMMON_DIR = benchmark/common

# for host
HOST_GCC = /usr/bin/gcc
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
PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)
COMMON_SRCS = $(wildcard $(COMMON_DIR)/*.c) $(wildcard $(COMMON_DIR)/*.S)

# -fstack-usage
RISCV_CFLAGS	+=	$(ARCH_FLAGS) \
					-ffunction-sections -fdata-sections \
					-I$(abspath $(BSP_DIR)/install/include/) -I$(COMMON_DIR) -I$(SRC_DIR) \
					--specs=$(SPEC).specs \
					-DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF) \
					-O3 
HOST_CFLAGS 	= 	-Wall -Wextra -Wmissing-prototypes -Wredundant-decls \
					-fomit-frame-pointer -march=native \
					-I$(abspath $(BSP_DIR)/install/include/) -I$(COMMON_DIR) -I$(SRC_DIR) \
					-O3

#					-Xlinker --defsym=__stack_size=0x2790 for kem.elf, 0x1a40 for stack.elf \
#					-Xlinker --defsym=__heap_max=1
RISCV_LDFLAGS	+=	-Wl,--gc-sections -Wl,-Map,$(basename $@).map \
					-nostartfiles -nostdlib \
					-L$(sort $(dir $(abspath $(filter %.a,$^)))) \
					-T$(abspath $(filter %.lds,$^)) \
					-Xlinker --defsym=__stack_size=0x2790 \
					-Xlinker --defsym=__heap_max=1

RISCV_LDLIBS	+=	-Wl,--start-group -lc -lgcc -lm -lmetal -lmetal-gloss -Wl,--end-group

.PHONY: host
host: host_out/kem

host_out/kem: \
		benchmark/kem.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS)
	mkdir -p $(dir $@)
	$(HOST_GCC) $(HOST_CFLAGS) -o $@ $(filter %.c,$^)

# out/PQCgenKAT_kem.elf out/test_kex.elf out/kem.elf out/stack.elf
.PHONY: all
all: out/kem.elf

# $(RISCV_GCC) -o $(basename $@) $(RISCV_CFLAGS) \
# 	$(filter %.c,$^) $(filter %.S,$^) -I$(COMMON_DIR) -I$(SRC_DIR) $(RISCV_LDFLAGS)
# mv $(basename $@) $@

out/%.elf: \
		benchmark/%.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS) \
		benchmark/common/libmetal.a \
		benchmark/common/libmetal-gloss.a \
		benchmark/common/metal.default.lds
	mkdir -p $(dir $@)
	$(RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_LDFLAGS) \
		$(filter %.c,$^) $(filter %.S,$^) \
		$(RISCV_LDLIBS) -o $(basename $@)
	mv $(basename $@) $@
	touch -c $@
	$(RISCV_OBJDUMP) --source --all-headers --demangle --line-numbers --wide $@ > $(basename $@).lst
	$(RISCV_SIZE) $@
	$(RISCV_OBJCOPY) -O ihex $@ $(basename $@).hex
	$(RISCV_OBJDUMP) -d $@ > $(basename $@).s

# cat *.su > $(basename $@).stack
# rm *.su

.PHONY: clean-software
clean-software:
	rm -rf out

.PHONY: clean
clean: clean-software