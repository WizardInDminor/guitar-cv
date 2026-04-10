TARGET = guitar-cv

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -g -Wall -Wextra -ffreestanding -nostdlib

LDFLAGS = -T ld/stm32f407.ld -Wl,--gc-sections -Wl,-Map=$(TARGET).map

SRC = $(wildcard src/*.c)
STARTUP = $(wildcard startup/*.s)

OBJ = $(patsubst src/%.c,build/%.o,$(SRC)) \
      $(patsubst startup/%.s,build/%.o,$(STARTUP))

all: $(TARGET).elf $(TARGET).bin

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: startup/%.s | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

$(TARGET).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).bin

flash:
	openocd -f interface/stlink.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program guitar-cv.elf verify reset exit"

clean:
	rm -f build/*.o $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all flash clean
