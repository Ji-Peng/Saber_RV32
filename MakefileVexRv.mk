CROSS_PREFIX ?= riscv64-unknown-elf
CC            = $(CROSS_PREFIX)-gcc
LD            = $(CROSS_PREFIX)-gcc
OBJCOPY       = $(CROSS_PREFIX)-objcopy
OBJDUMP 	  = $(CROSS_PREFIX)-objdump
PLATFORM     ?= vexriscv

COMMON_DIR = benchmark/common
SRC_DIR = src

RISCV_ARCH ?= rv32im
RISCV_ABI ?= ilp32
RISCV_CMODEL ?= medany
RISCV_ARCHFLAGS += -march=$(RISCV_ARCH)
RISCV_ARCHFLAGS += -mabi=$(RISCV_ABI)
RISCV_ARCHFLAGS += -mcmodel=$(RISCV_CMODEL)

# C Flags that must be used for the Murax SoC
VEXRISCV_CFLAGS += $(RISCV_ARCHFLAGS)
VEXRISCV_CFLAGS += -fstrict-volatile-bitfields

# VEXRISCV_PLATFORM ?= pqvexriscvup5k
VEXRISCV_PLATFORM ?= pqvexriscvsim

PLATFORM_BSP_DIR = bsp/vexrv

VEXRISCV_LINKERSCRIPT = $(PLATFORM_BSP_DIR)/$(VEXRISCV_PLATFORM).ld
# LD Flags that must be used to link executables for the Murax SoC
VEXRISCV_LDFLAGS += $(RISCV_ARCHFLAGS)
VEXRISCV_LDFLAGS += --specs=nosys.specs
VEXRISCV_LDFLAGS += -Wl,-T$(VEXRISCV_LINKERSCRIPT)
VEXRISCV_LDFLAGS += -nostartfiles -ffreestanding -Wl,--gc-sections
VEXRISCV_LDFLAGS += -L$(PLATFORM_BSP_DIR)
VEXRISCV_LDFLAGS += -Wl,--start-group -l$(VEXRISCV_PLATFORM)bsp -lc -Wl,--end-group

PLATFORM_CFLAGS = $(VEXRISCV_CFLAGS) -DVEXRISCV_PLATFORM=$(VEXRISCV_PLATFORM)
PLATFORM_LDFLAGS = $(VEXRISCV_LDFLAGS)
PLATFORM_LINKDEP = $(PLATFORM_BSP_DIR)/lib$(VEXRISCV_PLATFORM)bsp.a $(VEXRISCV_LINKERSCRIPT)

$(PLATFORM_LINKDEP):
	make -C $(PLATFORM_BSP_DIR) PLATFORM=$(VEXRISCV_PLATFORM)

CFLAGS       += -O3
CFLAGS       += -fno-common -MD $(DEFINES) \
							-DPQRISCV_PLATFORM=$(PLATFORM) \
              $(PLATFORM_CFLAGS)
CFLAGS		+= -I$(PLATFORM_BSP_DIR) -I$(COMMON_DIR) -I$(SRC_DIR)		  			  
LDFLAGS      += \
                $(PLATFORM_LDFLAGS)

CC_HOST      = gcc
LD_HOST      = gcc
CFLAGS_HOST  = -O3 -Wall -Wextra -Wpedantic
LDFLAGS_HOST =

PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)
COMMON_SRCS = $(COMMON_DIR)/aes.c $(COMMON_DIR)/fips202.c $(COMMON_DIR)/rng.c $(COMMON_DIR)/hal-vexriscv.c
COMMONINCLUDES=-I"benchmark/common"

.PHONY: all
all: out/kem_vexrv.elf out/stack_vexrv.elf

out/%.elf: \
		benchmark/%.c \
		$(COMMON_SRCS) $(PROGRAM_SRCS) \
		$(PLATFORM_LINKDEP)
	mkdir -p $(dir $@)
	$(CC) -o $@ $(CFLAGS) \
		$(filter %.c,$^) $(filter %.S,$^) \
		$(LDFLAGS)
	$(OBJCOPY) -O ihex $@ $(basename $@).hex
	$(OBJCOPY) -Obinary $@ $(basename $@).bin
	$(OBJDUMP) -D $@ > $(basename $@).s

.PHONY: clean-software
clean-software:
	rm -rf out
	rm -rf host_out

.PHONY: clean
clean: clean-software
