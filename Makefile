PROGRAM = hello
TARGET = sifive-hifive1-revb
# set E_SDK_TAGS, E_SDK_REQS
include freedom-e-sdk.mk

# The configuration defaults to Debug. Valid choices are: debug and release
CONFIGURATION ?= debug

#############################################################
# Makefile Arguments
#############################################################

# BSP_DIR sets the path to the target-specific board support package.
BSP_DIR ?= $(abspath bsp)
# SRC_DIR sets the path to the program source directory
SRC_DIR ?= $(abspath src)
# Set FREEDOM_E_SDK_VENV_PATH to use a project-local virtualenv
export FREEDOM_E_SDK_VENV_PATH ?=  $(abspath .)/venv
# Set FREERTOS_METAL_VENV_PATH to use same venv as FREEDOM_E_SDK_VENV_PATH
export FREERTOS_METAL_VENV_PATH ?= $(FREEDOM_E_SDK_VENV_PATH)

# Include the BSP settings ARCH, ABI etc
include $(BSP_DIR)/settings.mk

LIBMETAL_EXTRA=-lmetal-gloss
METAL_WITH_EXTRA=--with-builtin-libgloss
SPEC=nano

LINK_TARGET = default

MTIME_RATE_HZ_DEF=32768

#############################################################
# Toolchain
#############################################################

# Allow users to select a different cross compiler.
CROSS_COMPILE ?= riscv64-unknown-elf

# If users don't specify RISCV_PATH then assume that the tools will just be in
# their path.
RISCV_GCC     := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-gcc)
RISCV_OBJDUMP := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-objdump)
RISCV_OBJCOPY := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-objcopy)
RISCV_GDB     := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-gdb)
RISCV_AR      := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-ar)
RISCV_SIZE    := $(abspath $(RISCV_PATH)/bin/$(CROSS_COMPILE)-size)
PATH          := $(abspath $(RISCV_PATH)/bin):$(PATH)

SEGGER_JLINK_EXE := JLinkExe
SEGGER_JLINK_GDB_SERVER := JLinkGDBServer

#############################################################
# Software Flags
#############################################################

ARCH_FLAGS = -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=$(RISCV_CMODEL) 

RISCV_CFLAGS  	+= 	-O3 \
					-ffunction-sections -fdata-sections \
					-I$(abspath $(BSP_DIR)/install/include/) \
					--specs=$(SPEC).specs \
					-DMTIME_RATE_HZ_DEF=$(MTIME_RATE_HZ_DEF) \
					$(ARCH_FLAGS)

# Turn on garbage collection for unused sections
# Turn on linker map file generation
# Turn off the C standard library
# Find the archive files and linker scripts
RISCV_LDFLAGS 	+= 	-Wl,--start-group  -lc -lgcc -lm -lmetal $(LIBMETAL_EXTRA) -Wl,--end-group \
					-Wl,-Map,$(PROGRAM).map \
					-T$(abspath $(filter %.lds,$^)) -nostartfiles -nostdlib -Wl,--gc-sections \
					$(ARCH_FLAGS) \
					-L$(sort $(dir $(abspath $(filter %.a,$^))))
					
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

$(PROGRAM_ELF): \
		$(PROGRAM_SRCS) \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal.a \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal-gloss.a \
		$(BSP_DIR)/metal.$(LINK_TARGET).lds
	mkdir -p $(dir $@)
	$(RISCV_GCC) -o $(basename $(notdir $@)) $(RISCV_CFLAGS) \
		$(PROGRAM_SRCS) \
		$(RISCV_LDFLAGS)
	mv $(basename $(notdir $@)) $@
	mv $(basename $(notdir $@)).map $(dir $@)
	touch -c $@
	$(RISCV_OBJDUMP) --source --all-headers --demangle --line-numbers --wide $@ > $(PROGRAM_LST)
	$(RISCV_SIZE) $@

$(PROGRAM_HEX): \
		$(PROGRAM_ELF)
	$(RISCV_OBJCOPY) -O ihex $(PROGRAM_ELF) $@

.PHONY: clean-software
clean-software:
	$(MAKE) -C $(SRC_DIR) clean
	rm -rf $(SRC_DIR)/$(CONFIGURATION)
.PHONY: clean
clean: clean-software

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