BSP_DIR ?= $(abspath bsp)
得到bsp文件夹的绝对路径，并赋值给BSP_DIR

export FREERTOS_SOURCE_PATH = $(abspath FreeRTOS-metal)
export出的变量会被make的子进程继承

wildcard: 通配符
如$(wildcard *.c)能匹配所有.c文件

notdir: 去除路径名

patsubst函数，模式替换：
```bash
ifeq ($(patsubst rv32%,rv32,$(RISCV_ARCH)),rv32)
RISCV_XLEN := 32
else ifeq ($(patsubst rv64%,rv64,$(RISCV_ARCH)),rv64)
RISCV_XLEN := 64
else
$(error Unable to determine XLEN from $(RISCV_ARCH))
endif
```

使用环境变量直接$(RISCV_PATH)即可

为何有那么多FLAGS呢？RISCV_CCASFLAGS, RISCV_CFLAGS, RISCV_CXXFLAGS???
最后只保留了CFLAGS, ASFLAGS, LDFLAGS, LDLIBS，CXX是给CPP用的，其余的可忽略

另外从最后的编译命令来看：
gcc CFLAGS LDFLAGS input_file LDLIBS -o output_file
实际上AS也没用到啊

Saber on M4的编译参数如下：
```bash
ARCH_FLAGS  = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
DEFINES     = -DSTM32F4

CFLAGS     += -O3 \
              -Wall -Wextra -Wimplicit-function-declaration \
              -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
              -Wundef -Wshadow \
              -I$(OPENCM3DIR)/include \
              -fno-common $(ARCH_FLAGS) -MD $(DEFINES)
LDFLAGS    += --static -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
              -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections \
               $(ARCH_FLAGS) -L$(OPENCM3DIR)/lib -lm -l$(OPENCM3NAME)
```
