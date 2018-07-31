
ARCH := x86_64-pc-elf

CC := $(ARCH)-gcc
CFLAGS := -std=c1x -O3 -Wall -Wextra -pedantic -Wno-unused-parameter -I. -ffreestanding -mno-red-zone -mcmodel=kernel -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -mno-mmx -g

AS := nasm
ASFLAGS := -f elf64 -g -F dwarf

LD := $(ARCH)-ld
LDFLAGS := -z max-page-size=0x1000

AR := $(ARCH)-ar
ARFLAGS := 

TARGET := libitskernel.a
SOURCES := $(shell find -name "*.c" -or -name "*.s" -type f)
OBJECTS := $(addsuffix .o, $(basename $(SOURCES)))
DEPENDENCIES := $(shell find -name "*.d")

.PHONY: all clean

all: $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJECTS) $(DEPENDENCIES)

$(TARGET): $(OBJECTS)
	 $(AR) rcs $@ $(ARFLAGS) $^

include $(DEPENDENCIES)

.c.o:
	$(CC) $(CFLAGS) -MD -MP -MT $@ -MF $(addsuffix .d,$(basename $@)) -c -o $@ $<

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
