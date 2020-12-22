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
