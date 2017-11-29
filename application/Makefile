mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

SRC_PATH := ./src
INC_PATH := ./inc
OUT_PATH := ./out
OBJ_PATH := $(OUT_PATH)/obj

SYMBOLS  := -DDEBUG -DCORE_M4 -D__USE_LPCOPEN -D__LPC43XX__ -D__CODE_RED
INCLUDES := -I./inc -I../lpcopen_lpc4337/inc -I../lpcopen_lpc4337/inc/usbd -I../sapi/inc
CFLAGS   := -Wall -ggdb3 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -c

LFLAGS   := -nostdlib -fno-builtin -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Xlinker -Map=$(OUT_PATH)/$(PROJECT).map
LIB_PATH := -L../lpcopen_lpc4337/out -L../sapi/out
LIBS	 := -llpcopen_lpc4337 -lsapi
LD_FILE  := -T./ld/ciaa_lpc4337.ld

SRC_FILES := $(wildcard $(SRC_PATH)/*.c)
ASM_FILES := $(wildcard $(SRC_PATH)/*.S)
INC_FILES := $(wildcard $(INC_PATH)/*.h)
OBJ_FILES := $(subst $(SRC_PATH),$(OBJ_PATH),$(SRC_FILES:.c=.o))
OBJ_FILES += $(subst $(SRC_PATH),$(OBJ_PATH),$(ASM_FILES:.S=.o))

$(PROJECT): $(OBJ_FILES)
	@echo "*** Linking project $(PROJECT) ***"
	arm-none-eabi-gcc $(LIB_PATH) $(LFLAGS) $(LD_FILE) -o $(OUT_PATH)/$(PROJECT).axf $(OBJ_FILES) $(LIBS) 
	arm-none-eabi-size $(OUT_PATH)/$(PROJECT).axf
	arm-none-eabi-objcopy -v -O binary $(OUT_PATH)/$(PROJECT).axf $(OUT_PATH)/$(PROJECT).bin
	@echo ""

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@echo "*** Compiling C file $< ***"
	arm-none-eabi-gcc $(SYMBOLS) $(INCLUDES) $(CFLAGS) $< -o $@ 
	@echo ""

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.S
	@echo "*** Compiling Assembly file $< ***"
	arm-none-eabi-gcc $(SYMBOLS) $(INCLUDES) $(CFLAGS) $< -o $@
	@echo ""

clean:
	rm -f $(OBJ_PATH)/*.*
	rm -f $(OUT_PATH)/*.*
