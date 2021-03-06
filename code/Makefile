# Builds the given project parts.
# Usage: make TARGET
#     where TARGET is one or more of the following:
#     - kernel: Only build the kernel.
#     - lib: Only build the standard library.
#     - net: Only build the network library.
#     - ui: Only build the UI process.

# Cross compiler binary prefix
MAKEFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
ARCH := $(MAKEFILE_DIR)/../compiler/bin/x86_64-elf

# Compiler
# TODO add -O2 again
#    -> breaks stack traces
#    -> seems to corrupt network driver??
CC := $(ARCH)-gcc
CFLAGS := -std=c1x \
		  -Wall \
		  -Wextra \
		  -Wno-unused-parameter \
		  -pedantic \
		  -ffreestanding \
		  -mno-red-zone \
		  -g

# The kernel needs some special flags
CFLAGS_KERNEL := $(CFLAGS) \
		  -mno-sse \
		  -mno-sse2 \
		  -mno-sse3 \
		  -mno-3dnow \
		  -mno-mmx \


# Assembler
AS := nasm
ASFLAGS := -f elf64 -g -F dwarf

# Linker
LD := $(ARCH)-ld
LDFLAGS := -z max-page-size=0x1000 

# Archiver
AR := $(ARCH)-ar
ARFLAGS := 

# Find source files
KERNEL_SOURCES := $(shell find ./kernel/ -name "*.c" -or -name "*.s" -type f)
LIB_SOURCES := $(shell find ./lib/ -name "*.c" -or -path "./lib/*" -name "*.s" -type f)
NET_SOURCES := $(shell find ./net/ -not -path "./net/lwip/doc/*" -name "*.c" -or -path "./net/*" -name "*.s" -type f)
UI_SOURCES := $(shell find ./ui/ -name "*.c" -or -path "./ui/*" -name "*.s" -type f)

# Object files
KERNEL_OBJECTS := $(addsuffix .o, $(KERNEL_SOURCES))
LIB_OBJECTS := $(addsuffix .o, $(LIB_SOURCES))
NET_OBJECTS := $(addsuffix .o, $(NET_SOURCES))
UI_OBJECTS := $(addsuffix .o, $(UI_SOURCES))

# Dependency files (auto-generated by compiler)
DEPENDENCIES := $(shell find -name "*.d")

# Builds all targets.
.PHONY: all
all: kernel lib net ui

# Removes all generated files.
.PHONY: clean
clean:
	$(RM) $(DEPENDENCIES)
	$(RM) kernel.elf $(KERNEL_OBJECTS)
	$(RM) libkernel.a $(LIB_OBJECTS)
	$(RM) libnet.a $(NET_OBJECTS)
	$(RM) ui.elf $(UI_OBJECTS)

# Use compiler-generated dependency files
include $(DEPENDENCIES)

# Main targets
.PHONY: kernel lib net ui
kernel: $(KERNEL_OBJECTS)
	$(LD) $(LDFLAGS) -Tkernel/linker.lds -o kernel.elf $(KERNEL_OBJECTS)
lib: $(LIB_OBJECTS)
	$(AR) rcs libkernel.a $(ARFLAGS) $(LIB_OBJECTS)
net: lib $(NET_OBJECTS)
	$(AR) rcs libnet.a $(ARFLAGS) $(NET_OBJECTS)
ui: lib net $(UI_OBJECTS)
	$(LD) -o ui.elf $(UI_OBJECTS) $(LDFLAGS) -L. -l net -l kernel -Tui/linker.lds
	
# Rules for compilation
kernel/%.c.o: kernel/%.c
	$(CC) $(CFLAGS_KERNEL) -I./kernel/ -mcmodel=kernel -MD -MP -MT $@ -MF $(addsuffix .d,$(basename $@)) -c -o $@ $<
kernel/%.s.o: kernel/%.s
	$(AS) $(ASFLAGS) -o $@ $<
lib/%.c.o: lib/%.c
	$(CC) $(CFLAGS) -I./lib/ -mcmodel=small -MD -MP -MT $@ -MF $(addsuffix .d,$(basename $@)) -c -o $@ $<
lib/%.s.o: lib/%.s
	$(AS) $(ASFLAGS) -o $@ $<
net/%.c.o: net/%.c
	$(CC) $(CFLAGS) -I./lib/ -I./net/itslwip/ -I./net/lwip/src/include/ -mcmodel=small -MD -MP -MT $@ -MF $(addsuffix .d,$(basename $@)) -c -o $@ $<
net/%.s.o: net/%.s
	$(AS) $(ASFLAGS) -o $@ $<
ui/%.c.o: ui/%.c
	$(CC) $(CFLAGS) -I./lib/ -I./net/itslwip/ -mcmodel=small -MD -MP -MT $@ -MF $(addsuffix .d,$(basename $@)) -c -o $@ $<
ui/%.s.o: ui/%.s
	$(AS) $(ASFLAGS) -o $@ $<
