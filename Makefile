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
RISCV_CFLAGS  	+= 	-O3 \
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
all: out/bench.elf

PROGRAM_SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/*.h) $(wildcard $(SRC_DIR)/*.S)

out/%.elf: \
		benchmark/%.c $(PROGRAM_SRCS) \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal.a \
		$(BSP_DIR)/install/lib/$(CONFIGURATION)/libmetal-gloss.a \
		$(BSP_DIR)/metal.default.lds
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

#############################################################
# Freedom Studio
#############################################################
include $(BSP_DIR)/settings.mk
include freedom-e-sdk.mk
TARGET = sifive-hifive1-revb
export FREERTOS_SOURCE_PATH = $(abspath FreeRTOS-metal)
# Set FREEDOM_E_SDK_VENV_PATH to use a project-local virtualenv
export FREEDOM_E_SDK_VENV_PATH ?=  $(abspath .)/venv
METAL_WITH_EXTRA=--with-builtin-libgloss

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

