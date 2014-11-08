TARGET = main

#set the path to STM32F429I-Discovery firmware package
STDP ?= ../STM32F429I-Discovery_FW_V1.0.1

EXECUTABLE = $(TARGET).elf
BIN_IMAGE = $(TARGET).bin
HEX_IMAGE = $(TARGET).hex
MAP_FILE = $(TARGET).map
LIST_FILE = $(TARGET).lst

# Toolchain configurations
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

#Folder Setting
FREERTOS_SRC = freertos
FREERTOS_INC = $(FREERTOS_SRC)/include/    

TOOLDIR = tool
BUILDDIR = build
OUTDIR = build/$(TARGET)
INCDIR = include \
		 CORTEX_M4F_STM32F4 \
         $(FREERTOS_INC) \
		 $(FREERTOS_SRC)/portable/GCC/ARM_CM4F \
	  	 $(STDP)/Libraries/CMSIS/Device/ST/STM32F4xx/Include \
	  	 $(STDP)/Libraries/CMSIS/Include \
	  	 $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/inc \
	  	 $(STDP)/Utilities/STM32F429I-Discovery

SRCDIR = src \
      	 $(FREERTOS_SRC) \
    	 $(STDP)/Libraries/STM32F4xx_StdPeriph_Driver/src \
#    	 $(STDP)/Utilities/STM32F429I-Discovery

INCLUDES = $(addprefix -I,$(INCDIR))

HEAP_IMPL = heap_1
SRC = $(wildcard $(addsuffix /*.c,$(SRCDIR))) \
	  $(wildcard $(addsuffix /*.s,$(SRCDIR))) \
	  $(FREERTOS_SRC)/portable/MemMang/$(HEAP_IMPL).c \
	  $(FREERTOS_SRC)/portable/GCC/ARM_CM4F/port.c \
	  CORTEX_M4F_STM32F4/startup_stm32f429_439xx.s \
	  CORTEX_M4F_STM32F4/startup/system_stm32f4xx.c
OBJ := $(addprefix $(OUTDIR)/,$(patsubst %.s,%.o,$(SRC:.c=.o)))
DEP = $(OBJ:.o=.o.d)

# Cortex-M4 implements the ARMv7E-M architecture
CPU = cortex-m4
CFLAGS = -mcpu=$(CPU) -march=armv7e-m -mtune=cortex-m4
CFLAGS += -mlittle-endian -mthumb
# Need study
CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp

define get_library_path
    $(shell dirname $(shell $(CC) $(CFLAGS) -print-file-name=$(1)))
endef
LDFLAGS += -L $(call get_library_path,libc.a)
LDFLAGS += -L $(call get_library_path,libgcc.a)

# Basic configurations
CFLAGS += -g3 -std=c99
CFLAGS += -Wall -Werror
CFLAGS += -DUSER_NAME=\"$(USER)\"

# Optimizations
CFLAGS += -g -std=c99 -O0 -ffast-math
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections
CFLAGS += -fno-common
CFLAGS += --param max-inline-insns-single=1000

# specify STM32F429
CFLAGS += -DSTM32F429_439xx

# to run from FLASH
CFLAGS += -DVECT_TAB_FLASH
LDFLAGS += -TCORTEX_M4F_STM32F4/stm32f429zi_flash.ld

# STM32F4xx_StdPeriph_Driver
CFLAGS += -DUSE_STDPERIPH_DRIVER
CFLAGS += -D"assert_param(expr)=((void)0)"

all: $(OUTDIR)/$(BIN_IMAGE) $(OUTDIR)/$(LIST_FILE)

$(OUTDIR)/$(BIN_IMAGE): $(OUTDIR)/$(EXECUTABLE)
	@echo "    OBJCOPY "$@
	@$(CROSS_COMPILE)objcopy -Obinary $< $@

$(OUTDIR)/$(LIST_FILE): $(OUTDIR)/$(EXECUTABLE)
	@echo "    LIST    "$@
	@$(CROSS_COMPILE)objdump -S $< > $@

$(OUTDIR)/$(EXECUTABLE): $(OBJ) $(DAT)
	@echo "    LD      "$@
	@echo "    MAP     "$(OUTDIR)/$(MAP_FILE)
	@$(CROSS_COMPILE)gcc $(CFLAGS) $(LDFLAGS) -Wl,-Map=$(OUTDIR)/$(MAP_FILE) -o $@ $^

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<

$(OUTDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@$(CROSS_COMPILE)gcc $(CFLAGS) -MMD -MF $@.d -o $@ -c $(INCLUDES) $<

.PHONY: clean
clean:
	rm -rf $(OUTDIR) $(TMPDIR)

.PHONY: clean-all
clean-all:
	rm -rf $(BUILDDIR) $(TMPDIR)

-include $(DEP)

flash:
	st-flash write $(OUTDIR)/$(TARGET).bin 0x8000000

openocd_flash:
	openocd \
	-f board/stm32f429discovery.cfg \
	-c "init" \
	-c "reset init" \
	-c "flash probe 0" \
	-c "flash info 0" \
	-c "flash write_image erase $(OUTDIR)/$(TARGET).bin 0x08000000" \
	-c "reset run" -c shutdown

dbg: $(OUTDIR)/$(BIN_IMAGE) 
	openocd -f board/stm32f429discovery.cfg >/dev/null & \
    echo $$! > $(OUTDIR)/openocd_pid && \
    $(CROSS_COMPILE)gdb -x $(TOOLDIR)/gdbscript && \
    cat $(OUTDIR)/openocd_pid |`xargs kill 2>/dev/null || test true` && \
    rm -f $(OUTDIR)/openocd_pid

cdbg: $(OUTDIR)/$(BIN_IMAGE) 
	openocd -f board/stm32f429discovery.cfg >/dev/null & \
    echo $$! > $(OUTDIR)/openocd_pid && \
    cgdb -d $(CROSS_COMPILE)gdb -x $(TOOLDIR)/gdbscript && \
    cat $(OUTDIR)/openocd_pid |`xargs kill 2>/dev/null || test true` && \
    rm -f $(OUTDIR)/openocd_pid
