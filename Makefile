PROGRAM = hello
TARGET = sifive-hifive1-revb
# set E_SDK_TAGS, E_SDK_REQS
include freedom-e-sdk.mk

# The configuration defaults to Debug. Valid choices are:
#   - debug
#   - release
CONFIGURATION ?= debug

#############################################################
# Makefile Arguments
#############################################################

# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR ?= $(abspath bsp)
# SRC_DIR sets the path to the program source directory
SRC_DIR ?= $(abspath src)
# FREERTOS_SOURCE_PATH sets the path to the FreeRTOS source directory
export FREERTOS_SOURCE_PATH = $(abspath FreeRTOS-metal)
# Set FREEDOM_E_SDK_VENV_PATH to use a project-local virtualenv
export FREEDOM_E_SDK_VENV_PATH ?=  $(abspath .)/venv
# Set FREERTOS_METAL_VENV_PATH to use same venv as FREEDOM_E_SDK_VENV_PATH
export FREERTOS_METAL_VENV_PATH ?= $(FREEDOM_E_SDK_VENV_PATH)
# SCL_SOURCE_PATH sets the path to the SCL source directory
export SCL_SOURCE_PATH = $(abspath scl-metal)


#############################################################
# BSP loading
#############################################################

# There must be a settings makefile fragment in the BSP's board directory.
ifeq ($(wildcard $(BSP_DIR)/settings.mk),)
$(error Unable to find BSP for $(TARGET), expected to find $(BSP_DIR)/settings.mk)
endif

# Include the BSP settings ARCH, ABI etc
include $(BSP_DIR)/settings.mk

# Check that settings.mk sets RISCV_ARCH and RISCV_ABI
ifeq ($(RISCV_ARCH),)
$(error $(BSP_DIR)/board.mk must set RISCV_ARCH, the RISC-V ISA string to target)
endif

ifeq ($(RISCV_ABI),)
$(error $(BSP_DIR)/board.mk must set RISCV_ABI, the ABI to target)
endif

ifeq ($(RISCV_CMODEL),)
RISCV_CMODEL = medany
endif

ifeq ($(RISCV_LIBC),)
RISCV_LIBC=nano
endif

ifeq ($(RISCV_LIBC),segger)
# Disable format string errors when building with -Werror
RISCV_CFLAGS += -Wno-error=format=

LIBMETAL_EXTRA=-lmetal-segger
METAL_WITH_EXTRA=--with-builtin-libmetal-segger
SPEC=gloss-segger
endif

ifeq ($(RISCV_LIBC),nano)
LIBMETAL_EXTRA=-lmetal-gloss
METAL_WITH_EXTRA=--with-builtin-libgloss
SPEC=nano
endif

ifeq ($(SPEC),)
$(error RISCV_LIBC set to an unsupported value: $(RISCV_LIBC))
endif

ifeq ($(PROGRAM),dhrystone)
ifeq ($(LINK_TARGET),)
  ifneq ($(TARGET),freedom-e310-arty)
  ifneq ($(TARGET),sifive-hifive1)
  ifneq ($(TARGET),sifive-hifive1-revb)
    LINK_TARGET = ramrodata
  endif
  endif
  endif
endif
endif

ifeq ($(PROGRAM),coremark)
ifeq ($(LINK_TARGET),)
LINK_TARGET = ramrodata
endif
endif

ifneq ($(findstring freertos,$(PROGRAM)),)
ifeq ($(LINK_TARGET),)
LINK_TARGET = freertos
endif
endif

ifeq ($(LINK_TARGET),)
LINK_TARGET = default
endif

# Determines the XLEN from the toolchain tuple
# rv32imac in bsp setting.mk
ifeq ($(patsubst rv32%,rv32,$(RISCV_ARCH)),rv32)
RISCV_XLEN := 32
else ifeq ($(patsubst rv64%,rv64,$(RISCV_ARCH)),rv64)
RISCV_XLEN := 64
else
$(error Unable to determine XLEN from $(RISCV_ARCH))
endif

MTIME_RATE_HZ_DEF=32768
ifeq ($(findstring qemu,$(TARGET)),qemu)
MTIME_RATE_HZ_DEF=10000000
else
ifeq ($(findstring unleashed,$(TARGET)),unleashed)
MTIME_RATE_HZ_DEF=1000000
endif
endif

#############################################################
# Toolchain
#############################################################

# Allow users to select a different cross compiler.
CROSS_COMPILE ?= riscv64-unknown-elf

# If users don't specify RISCV_PATH then assume that the tools will just be in
# their path.
ifeq ($(RISCV_PATH),)
RISCV_GCC     := $(CROSS_COMPILE)-gcc
RISCV_GXX     := $(CROSS_COMPILE)-g++
RISCV_OBJDUMP := $(CROSS_COMPILE)-objdump
RISCV_OBJCOPY := $(CROSS_COMPILE)-objcopy
RISCV_GDB     := $(CROSS_COMPILE)-gdb
RISCV_AR      := $(CROSS_COMPILE)-ar
RISCV_SIZE    := $(CROSS_COMPILE)-size
else
RISCV_GCC     := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-gcc)
RISCV_GXX     := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-g++)
RISCV_OBJDUMP := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-objdump)
RISCV_OBJCOPY := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-objcopy)
RISCV_GDB     := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-gdb)
RISCV_AR      := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-ar)
RISCV_SIZE    := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-size)
PATH          := $(abspath $(RISCV_PATH)/bin):$(PATH)
endif

SEGGER_JLINK_EXE := JLinkExe
SEGGER_JLINK_GDB_SERVER := JLinkGDBServer

QEMU_RISCV32 = qemu-system-riscv32
QEMU_RISCV64 = qemu-system-riscv64

#############################################################
# Software Flags
#############################################################

# Set the arch, ABI, and code model
RISCV_CCASFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=$(RISCV_CMODEL)
RISCV_CFLAGS    += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=$(RISCV_CMODEL)
RISCV_CXXFLAGS  += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=$(RISCV_CMODEL)
RISCV_ASFLAGS   += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=$(RISCV_CMODEL)

# Prune unused functions and data
ifeq ($(RISCV_SERIES),sifive-8-series)
ifeq ($(PROGRAM),dhrystone)
RISCV_CFLAGS   += -fno-function-sections -fno-data-sections
RISCV_CXXFLAGS += -fno-function-sections -fno-data-sections
else
RISCV_CFLAGS   += -ffunction-sections -fdata-sections
RISCV_CXXFLAGS += -ffunction-sections -fdata-sections
endif
else
RISCV_CFLAGS   += -ffunction-sections -fdata-sections
RISCV_CXXFLAGS += -ffunction-sections -fdata-sections
endif

# Include the Metal headers
RISCV_CCASFLAGS += -I$(abspath $(BSP_DIR)/install/include/)
RISCV_CFLAGS    += -I$(abspath $(BSP_DIR)/install/include/)
RISCV_CXXFLAGS  += -I$(abspath $(BSP_DIR)/install/include/)

# Reference selected library, using reduced C lib to replace standard lib
RISCV_ASFLAGS   += --specs=$(SPEC).specs
RISCV_CCASFLAGS += --specs=$(SPEC).specs
RISCV_CFLAGS    += --specs=$(SPEC).specs
RISCV_CXXFLAGS  += --specs=$(SPEC).specs

# Set the MTIME frequency
RISCV_CFLAGS    += -DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF)
RISCV_CXXFLAGS  += -DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF)

# Turn on garbage collection for unused sections
RISCV_LDFLAGS += -Wl,--gc-sections
# Turn on linker map file generation
RISCV_LDFLAGS += -Wl,-Map,$(PROGRAM).map
# Turn off the C standard library
RISCV_LDFLAGS += -nostartfiles -nostdlib
# Find the archive files and linker scripts
RISCV_LDFLAGS += -L$(sort $(dir $(abspath $(filter %.a,$^)))) -T$(abspath $(filter %.lds,$^))

# Link to the relevant libraries
RISCV_LDLIBS += -Wl,--start-group -lc -lgcc -lm -lmetal $(LIBMETAL_EXTRA) -Wl,--end-group

# Load the configuration Makefile
CONFIGURATION_FILE = $(wildcard $(CONFIGURATION).mk)
ifeq ($(words $(CONFIGURATION_FILE)),0)
$(error Unable to find the Makefile $(CONFIGURATION).mk for CONFIGURATION=$(CONFIGURATION))
endif
include $(CONFIGURATION).mk

# Load the instantiation Makefile
INSTANTIATION_FILE = $(wildcard $(SRC_DIR)/options.mk)
ifneq ($(words $(INSTANTIATION_FILE)),0)
include $(SRC_DIR)/options.mk
endif

# Benchmark CFLAGS go after loading the CONFIGURATION so that they can override the optimization level

# Checking if we use gcc-10 or not, which need different compiler options for better benchmark scores
GCC_VER_GTE10 := $(shell echo `${RISCV_GCC} -dumpversion | cut -f1-2 -d.` \>= 10 | bc )

ifeq ($(PROGRAM),dhrystone)
ifeq ($(DHRY_OPTION),)
# Ground rules (default)
RISCV_XCFLAGS += -mexplicit-relocs -fno-inline
else ifeq ($(DHRY_OPTION),fast)
# With inline and without lto
RISCV_XCFLAGS += -mexplicit-relocs -finline
else ifeq ($(DHRY_OPTION),best)
# Best Score
RISCV_XCFLAGS += -finline -flto -fwhole-program
endif
RISCV_XCFLAGS += -DDHRY_ITERS=$(TARGET_DHRY_ITERS)
endif

ifeq ($(PROGRAM),coremark)
ifeq ($(RISCV_SERIES),$(filter $(RISCV_SERIES),sifive-7-series sifive-8-series))
# 8-series currently uses 7-series mtune, but this may change
RISCV_XCFLAGS += -O2 -fno-common -funroll-loops -finline-functions -funroll-all-loops -falign-functions=8 -falign-jumps=8 -falign-loops=8 -finline-limit=1000 -mtune=sifive-7-series -ffast-math
else
ifeq ($(RISCV_XLEN),32)
RISCV_XCFLAGS += -O2 -fno-common -funroll-loops -finline-functions -falign-functions=16 -falign-jumps=4 -falign-loops=4 -finline-limit=1000 -fno-if-conversion2 -fselective-scheduling -fno-tree-dominator-opts -fno-reg-struct-return -fno-rename-registers --param case-values-threshold=8 -fno-crossjumping -freorder-blocks-and-partition -fno-tree-loop-if-convert -fno-tree-sink -fgcse-sm -fno-strict-overflow
else
RISCV_XCFLAGS += -O2 -fno-common -funroll-loops -finline-functions -falign-functions=16 -falign-jumps=4 -falign-loops=4 -finline-limit=1000 -fno-if-conversion2 -fselective-scheduling -fno-tree-dominator-opts
endif # RISCV_XLEN==32
endif # RISCV_SERIES==sifive-7-series|sifive-8-series
RISCV_XCFLAGS += -DITERATIONS=$(TARGET_CORE_ITERS)
ifeq ($(GCC_VER_GTE10),1)
# additional options for gcc-10 to get better performance
RISCV_XCFLAGS += -fno-tree-loop-distribute-patterns --param fsm-scale-path-stmts=3
endif # GCC_VER_GTE10==1
endif # PROGRAM==coremark

ifeq ($(findstring freertos,$(PROGRAM)),freertos)
RISCV_XCFLAGS += -DWAIT_MS=$(TARGET_FREERTOS_WAIT_MS)
endif

ifneq ($(filter rtl,$(TARGET_TAGS)),)
RISCV_XCFLAGS += -DHCA_BYPASS_TRNG
endif

# A method to pass cycle delay
RISCV_XCFLAGS += -DMETAL_WAIT_CYCLE=$(TARGET_INTR_WAIT_CYCLE)

#############################################################
# Software
#############################################################

PROGRAM_ELF ?= $(SRC_DIR)/$(CONFIGURATION)/$(PROGRAM).elf
PROGRAM_HEX ?= $(SRC_DIR)/$(CONFIGURATION)/$(PROGRAM).hex
PROGRAM_LST ?= $(SRC_DIR)/$(CONFIGURATION)/$(PROGRAM).lst

.PHONY: all
all: software

.PHONY: software
software: $(PROGRAM_ELF)

software: $(PROGRAM_HEX)

PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.h) $(wildcard $(SRC_DIR)/*.S)

# hello.elf
$(PROGRAM_ELF): \
		$(PROGRAM_SRCS) \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal.a \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal-gloss.a \
		$(BSP_DIR)/metal.$(LINK_TARGET).lds
	mkdir -p $(dir $@)
	# make -C src hello
	$(MAKE) -C $(SRC_DIR) $(basename $(notdir $@)) \
		PORT_DIR=$(PORT_DIR) \
		PROGRAM=$(PROGRAM) \
		AR=$(RISCV_AR) \
		CC=$(RISCV_GCC) \
		CXX=$(RISCV_GXX) \
		ASFLAGS="$(RISCV_ASFLAGS)" \
		CCASFLAGS="$(RISCV_CCASFLAGS)" \
		CFLAGS="$(RISCV_CFLAGS)" \
		CXXFLAGS="$(RISCV_CXXFLAGS)" \
		XCFLAGS="$(RISCV_XCFLAGS)" \
		LDFLAGS="$(RISCV_LDFLAGS)" \
		LDLIBS="$(RISCV_LDLIBS)" \
		FREERTOS_METAL_VENV_PATH="$(FREERTOS_METAL_VENV_PATH)"
	mv $(SRC_DIR)/$(basename $(notdir $@)) $@
	mv $(SRC_DIR)/$(basename $(notdir $@)).map $(dir $@)
	touch -c $@
	$(RISCV_OBJDUMP) --source --all-headers --demangle --line-numbers --wide $@ > $(PROGRAM_LST)
	$(RISCV_SIZE) $@

# Use elf2hex if we're creating a hex file for RTL simulation
ifneq ($(filter rtl,$(TARGET_TAGS)),)
.PHONY: software
$(PROGRAM_HEX): \
		scripts/elf2hex/install/bin/$(CROSS_COMPILE)-elf2hex \
		$(PROGRAM_ELF)
	$< --output $@ --input $(PROGRAM_ELF) --bit-width $(COREIP_MEM_WIDTH)
else
$(PROGRAM_HEX): \
		$(PROGRAM_ELF)
	$(RISCV_OBJCOPY) -O ihex $(PROGRAM_ELF) $@
endif


.PHONY: clean-software
clean-software:
	$(MAKE) -C $(SRC_DIR) PORT_DIR=$(PORT_DIR) clean
	rm -rf $(SRC_DIR)/$(CONFIGURATION)
.PHONY: clean
clean: clean-software

#############################################################
# elf2hex
#############################################################
scripts/elf2hex/build/Makefile: scripts/elf2hex/configure
	@rm -rf $(dir $@)
	@mkdir -p $(dir $@)
	cd $(dir $@); \
		$(abspath $<) \
		--prefix=$(abspath $(dir $<))/install \
		--target=$(CROSS_COMPILE)

scripts/elf2hex/install/bin/$(CROSS_COMPILE)-elf2hex: scripts/elf2hex/build/Makefile
	$(MAKE) -C $(dir $<) install
	touch -c $@

.PHONY: clean-elf2hex
clean-elf2hex:
	rm -rf scripts/elf2hex/build scripts/elf2hex/install
clean: clean-elf2hex

#############################################################
# Freedom Studio
#############################################################
.PHONY: list-standalone-info
list-standalone-info:
	@echo e-sdk-tags: $(E_SDK_TAGS)
	@echo e-sdk-reqs: $(E_SDK_REQS)
	@echo riscv-arch: $(RISCV_ARCH)
	@echo target-tags: $(TARGET_TAGS)
	@echo riscv-reqs: $(RISCV_REQS)
	@echo program-tags: $(PROGRAM_TAGS)

#############################################################
# Compiles an instance of Metal targeted at $(TARGET)
#############################################################
METAL_SOURCE_PATH ?= freedom-metal
METAL_HEADER	   = $(BSP_DIR)/metal.h
METAL_INLINE       = $(BSP_DIR)/metal-inline.h
PLATFORM_HEADER	   = $(BSP_DIR)/metal-platform.h

METAL_PREFIX       = $(abspath $(BSP_DIR)/install)
METAL_BUILD_DIR    = $(abspath $(BSP_DIR)/build/$(CONFIGURATION))
METAL_LIB_DIR	   = $(abspath $(BSP_DIR)/install/lib/$(CONFIGURATION))

METAL_HEADER_GENERATOR = freedom-metal_header-generator
BARE_HEADER_GENERATOR = freedom-bare_header-generator

OVERLAY_GENERATOR = scripts/devicetree-overlay-generator/generate_overlay.py
LDSCRIPT_GENERATOR = scripts/ldscript-generator/generate_ldscript.py
CMSIS_SVD_GENERATOR = scripts/cmsis-svd-generator/generate_svd.py
SETTINGS_GENERATOR = scripts/esdk-settings-generator/generate_settings.py

# Metal BSP file generation
#
# Requires devicetree compiler (dtc) and freedom-devicetree-tools to be in the
# PATH, otherwise the existing files are used.
#
# This allows user changes to the devicetree in $(BSP_DIR)/design.dts to be
# propagated through to the end application with a single invocation of Make

$(BSP_DIR)/design.dts: $(BSP_DIR)/core.dts $(OVERLAY_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(OVERLAY_GENERATOR) --type $(TARGET) --output $@ --rename-include $(notdir $<) $<

$(BSP_DIR)/metal.default.lds: $(BSP_DIR)/design.dts $(LDSCRIPT_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(LDSCRIPT_GENERATOR) -d $< -o $@

$(BSP_DIR)/metal.ramrodata.lds: $(BSP_DIR)/design.dts $(LDSCRIPT_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(LDSCRIPT_GENERATOR) -d $< -o $@ --ramrodata

$(BSP_DIR)/metal.scratchpad.lds: $(BSP_DIR)/design.dts $(LDSCRIPT_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(LDSCRIPT_GENERATOR) -d $< -o $@ --scratchpad

$(BSP_DIR)/metal.freertos.lds: $(BSP_DIR)/design.dts $(LDSCRIPT_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(LDSCRIPT_GENERATOR) -d $< -o $@ --freertos

$(BSP_DIR)/design.svd: $(BSP_DIR)/design.dts $(CMSIS_SVD_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(CMSIS_SVD_GENERATOR) -d $< -o $@

$(BSP_DIR)/settings.mk: $(BSP_DIR)/design.dts $(SETTINGS_GENERATOR)
	$(MAKE) -f scripts/virtualenv.mk virtualenv
	. $(FREEDOM_E_SDK_VENV_PATH)/bin/activate && $(SETTINGS_GENERATOR) -d $< -o $@ -t $(TARGET)

ifeq ($(findstring spike,$(TARGET)),spike)
$(BSP_DIR)/spike_options.sh:
	echo "export SPIKE_OPTIONS=\"\"" > $@

ifneq ($(shell which spike),)
$(BSP_DIR)/core.dts: $(BSP_DIR)/spike_options.sh
	. $< && scripts/spikedts $@
endif # which spike
endif # findstring spike,$(TARGET)

ifneq ($(shell which dtc),)
ifneq ($(shell which $(METAL_HEADER_GENERATOR)),)

$(BSP_DIR)/design.dtb: $(BSP_DIR)/design.dts
	cd $(dir $@) && dtc -I dts -O dtb -o $(notdir $@) $(notdir $<)

$(METAL_INLINE): $(BSP_DIR)/design.dtb
$(METAL_HEADER): $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(METAL_HEADER_GENERATOR) -d $(notdir $<) -o $(notdir $@)

$(PLATFORM_HEADER): $(BSP_DIR)/design.dtb
	cd $(dir $@) && $(BARE_HEADER_GENERATOR) -d $(notdir $<) -o $(notdir $@)

.PHONY: bsp
metal-bsp:\
	   $(METAL_HEADER) $(METAL_INLINE) $(PLATFORM_HEADER) \
	   $(BSP_DIR)/metal.default.lds \
	   $(BSP_DIR)/metal.ramrodata.lds \
	   $(BSP_DIR)/metal.scratchpad.lds \
	   $(BSP_DIR)/metal.freertos.lds \
	   $(BSP_DIR)/settings.mk
else
.PHONY: bsp
metal-bsp:
	@echo "Make cannot generate a BSP because it cannot find freedom-devicetree-tools"
	@exit 1
endif # which $(METAL_HEADER_GENERATOR)
else
.PHONY: bsp
metal-bsp:
	@echo "Make cannot generate a BSP because it cannot find dtc"
	@exit 1
endif # which dtc


.PHONY: metal
metal: $(METAL_LIB_DIR)/stamp

$(METAL_BUILD_DIR)/Makefile: \
	   $(METAL_HEADER) $(METAL_INLINE) $(PLATFORM_HEADER) \
	   $(BSP_DIR)/settings.mk
	@rm -rf $(dir $@)
	@mkdir -p $(dir $@)
	cd $(dir $@) && \
		CFLAGS="$(RISCV_CFLAGS)" \
		$(abspath $(METAL_SOURCE_PATH)/configure) \
		--host=$(CROSS_COMPILE) \
		--prefix=$(METAL_PREFIX) \
		--libdir=$(METAL_LIB_DIR) \
		$(METAL_WITH_EXTRA) \
		--with-machine-header=$(abspath $(METAL_HEADER)) \
		--with-machine-inline=$(abspath $(METAL_INLINE)) \
		--with-platform-header=$(abspath $(PLATFORM_HEADER))
	touch -c $@

$(METAL_LIB_DIR)/stamp: $(METAL_BUILD_DIR)/Makefile
	$(MAKE) -C $(METAL_BUILD_DIR) install
	date > $@

$(METAL_LIB_DIR)/lib%.a: $(METAL_LIB_DIR)/stamp ;@:

# If we're cleaning the last Metal library for a TARGET, then remove
# the install directory, otherwise just remove the built libs for that
# CONFIGURATION.
ifeq ($(words $(wildcard $(METAL_PREFIX)/lib/*)),1)
METAL_CLEAN = $(METAL_PREFIX)
else
METAL_CLEAN = $(METAL_LIB_DIR)
endif

.PHONY: clean-metal
clean-metal:
	rm -rf $(METAL_CLEAN)
	rm -rf $(METAL_BUILD_DIR)
clean: clean-metal

metal_install: metal
	$(MAKE) -C $(METAL_SOURCE_PATH) install

