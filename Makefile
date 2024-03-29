# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR = bsp/sifive
# SRC_DIR sets the path to the program source directory
SRC_DIR = src
COMMON_DIR = benchmark/common
HOST_DIR = benchmark/host

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
COMMON_SRCS = $(COMMON_DIR)/aes.c $(COMMON_DIR)/fips202.c $(COMMON_DIR)/rng.c $(COMMON_DIR)/getcycles.S
HOST_SRCS = $(wildcard $(HOST_DIR)/*.c)

RISCV_CFLAGS	+=	$(ARCH_FLAGS) \
					-ffunction-sections -fdata-sections \
					-I$(BSP_DIR)/install/include -I$(COMMON_DIR) -I$(SRC_DIR) \
					--specs=$(SPEC).specs \
					-DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF) \
					-Os
HOST_CFLAGS 	= 	-DHOST \
					-fomit-frame-pointer -fno-tree-vectorize \
					-I$(abspath $(BSP_DIR)/install/include/) -I$(COMMON_DIR) -I$(SRC_DIR) -I$(HOST_DIR) \
					-O3
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	HOST_CFLAGS += -march=native
endif
# no need to add -march=native on macos
ifeq ($(UNAME_S),Darwin)
	HOST_CFLAGS +=
endif
# for speed test: L=2: stack_size=0x1b00, L=3: stack_size=0x2300, L=4: stack_size=0x2550
# for stack test: L=2: stack_size=0x1a00, L=3: stack_size=0x1a00, L=4: stack_size=0x1a00
RISCV_LDFLAGS	+=	-Wl,--gc-sections -Wl,-Map,$(basename $@).map \
					-nostartfiles -nostdlib \
					-L$(sort $(dir $(abspath $(filter %.a,$^)))) \
					-T$(abspath $(filter %.lds,$^)) \
					-Xlinker --defsym=__stack_size=0x2300 \
					-Xlinker --defsym=__heap_max=1

RISCV_LDLIBS	+=	-Wl,--start-group -lc -lgcc -lm -lmetal -lmetal-gloss -Wl,--end-group

.PHONY: all
all: out/kem.elf

.PHONY: host
host: host_out/kem host_out/speed

out/%.elf: \
		benchmark/%.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS) \
		benchmark/common/libmetal.a \
		benchmark/common/libmetal-gloss.a \
		benchmark/common/metal.default.lds
	mkdir -p $(dir $@)
	$(RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_LDFLAGS) \
		$(filter %.c,$^) $(filter %.S,$^) \
		$(RISCV_LDLIBS) -o $@
	$(RISCV_OBJCOPY) -O ihex $@ $(basename $@).hex
	$(RISCV_OBJDUMP) -D $@ > $(basename $@).s

host_out/kem: \
		benchmark/kem.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS)
	mkdir -p $(dir $@)
	$(HOST_GCC) $(HOST_CFLAGS) -o $@ $(filter %.c,$^)

host_out/speed: \
		benchmark/host/speed_host.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS) $(HOST_SRCS)
	mkdir -p $(dir $@)
	$(HOST_GCC) $(HOST_CFLAGS) -o $@ $(filter %.c,$^)

.PHONY: clean-software
clean-software:
	rm -rf out
	rm -rf host_out

.PHONY: clean
clean: clean-software