mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT := $(notdir $(patsubst %/,%,$(dir $(mkfile_path))))

LPROJECT  := lib$(PROJECT).a

SRC_PATH  := ./src
INC_PATH  := ./inc
OUT_PATH  := ./out
OBJ_PATH  := $(OUT_PATH)/obj

SYMBOLS   := -DDEBUG -DCORE_M4 -D__USE_LPCOPEN -D__LPC43XX__ -D__CODE_RED
INCLUDES  := -I./inc -I../lpcopen_lpc4337/inc
CFLAGS    := -Wall -ggdb3 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -c

SRC_FILES := $(wildcard $(SRC_PATH)/*.c)
INC_FILES := $(wildcard $(INC_PATH)/*.h)
OBJ_FILES := $(subst $(SRC_PATH),$(OBJ_PATH),$(SRC_FILES:.c=.o))

$(LPROJECT): $(OBJ_FILES)
	@echo "*** Archiving project $(PROJECT) ***"
	arm-none-eabi-ar -r $(OUT_PATH)/$(LPROJECT) $(OBJ_FILES)
	arm-none-eabi-size $(OUT_PATH)/$(LPROJECT)
	@echo ""

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@echo "*** Compiling C file $< ***"
	arm-none-eabi-gcc $(SYMBOLS) $(INCLUDES) $(CFLAGS) $< -o $@ 
	@echo ""

clean:
	rm -f $(OBJ_PATH)/*.*
	rm -f $(OUT_PATH)/*.*

