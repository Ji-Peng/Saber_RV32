OPENCM3DIR  = ./libopencm3
OPENCM3NAME = opencm3_stm32f4
OPENCM3FILE = $(OPENCM3DIR)/lib/lib$(OPENCM3NAME).a
LDSCRIPT    = stm32f405x6.ld

PREFIX     ?= arm-none-eabi
CC          = $(PREFIX)-gcc
LD          = $(PREFIX)-gcc
OBJCOPY     = $(PREFIX)-objcopy
OBJDUMP		= $(PREFIX)-objdump

ARCH_FLAGS  = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

ifndef CRYPTO_ITERATIONS
CRYPTO_ITERATIONS=1
endif

DEFINES     = -DSTM32F4 -DCRYPTO_ITERATIONS=$(CRYPTO_ITERATIONS)

CFLAGS     += -O3 \
              -Wall -Wextra -Wimplicit-function-declaration \
              -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
              -Wundef -Wshadow \
              -I$(OPENCM3DIR)/include \
              -fno-common $(ARCH_FLAGS) -MD $(DEFINES) -DPLATFORM_M4

CC_HOST    = gcc
LD_HOST    = gcc
CFLAGS_HOST = -O3 -Wall -Wextra -Wpedantic
LDFLAGS_HOST = -lm
SRC_DIR = src
IMPLEMENTATION_INCLUDE = src

COMMONSOURCES= benchmark/common_m4/fips202.c benchmark/common_m4/keccakf1600.S

COMMONINCLUDES_M4=$(COMMONINCLUDES) -I"benchmark/common_m4"
RANDOMBYTES_M4=benchmark/common_m4/rng.c

DEST=bin
IMPLEMENTATION_SOURCES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.S)
IMPLEMENTATION_HEADERS = $(wildcard $(SRC_DIR)/*.h)

LDFLAGS    += --static -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
              -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections \
               $(ARCH_FLAGS) -L$(OPENCM3DIR)/lib -lm -l$(OPENCM3NAME)

.PHONY: all
all: $(DEST)/stack_m4.bin $(DEST)/kem_m4.bin

$(DEST)/%.bin: elf/%.elf
	mkdir -p $(DEST)
	$(OBJCOPY) -Obinary $^ $@
	$(OBJDUMP) -D $^ > $(basename $@).s

elf/%.elf: benchmark/%.c $(RANDOMBYTES_M4) $(IMPLEMENTATION_SOURCES) $(IMPLEMENTATION_HEADERS) $(OPENCM3FILE) benchmark/common_m4/hal-stm32f4.c
	mkdir -p elf
	$(CC) -o $@ $(CFLAGS) \
		$< $(COMMONSOURCES) $(RANDOMBYTES_M4) $(IMPLEMENTATION_SOURCES) benchmark/common_m4/hal-stm32f4.c \
		-I$(IMPLEMENTATION_INCLUDE) $(COMMONINCLUDES_M4) $(LDFLAGS)

$(OPENCM3FILE):
	@if [ ! "`ls -A $(OPENCM3_DIR)`" ] ; then \
		printf "######## ERROR ########\n"; \
		printf "\tlibopencm3 is not initialized.\n"; \
		printf "\tPlease run (in the root directory):\n"; \
		printf "\t$$ git submodule init\n"; \
		printf "\t$$ git submodule update\n"; \
		printf "\tbefore running make.\n"; \
		printf "######## ERROR ########\n"; \
		exit 1; \
		fi
	make -C $(OPENCM3DIR)

.PHONY: clean libclean

clean:
	rm -rf elf/
	rm -rf bin/
	rm -rf bin-host/
	rm -rf obj/
	rm -rf testvectors/
	rm -rf benchmarks/

libclean:
	make -C $(OPENCM3DIR) clean