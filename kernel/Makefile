# Nuke built-in rules and variables.
override MAKEFLAGS += -rR
override KERNEL := kernel

define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

override DEFAULT_CC := ../toolchain/x86_64-elf/bin/x86_64-elf-gcc
$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))
override DEFAULT_LD := ../toolchain/x86_64-elf/bin/x86_64-elf-ld
$(eval $(call DEFAULT_VAR,LD,$(DEFAULT_LD)))
override DEFAULT_CFLAGS := -g -O0 -pipe -lgcc
$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))
override DEFAULT_CPPFLAGS :=
$(eval $(call DEFAULT_VAR,CPPFLAGS,$(DEFAULT_CPPFLAGS)))
override DEFAULT_NASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,NASMFLAGS,$(DEFAULT_NASMFLAGS)))
override DEFAULT_LDFLAGS :=
$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

override CFLAGS += \
	-Werror \
    -Wall \
    -Wextra \
	-Wno-cast-function-type \
	-ffreestanding \
	-nostdlib \
	-flto \
    -fno-PIC \
    -std=gnu11 \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIE \
	-fno-omit-frame-pointer \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=kernel \
    -mno-red-zone \
	-ffunction-sections \
	-fdata-sections

CONDITIONALS :=	\
	-DKERNEL_BUILD

override CPPFLAGS := \
    -I ./include -I ../h64libc \
    $(CPPFLAGS) \
    -MMD \
    -MP

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    --no-dynamic-linker \
    -z text \
    -z max-page-size=0x1000 \
    -T linker.ld \
	-Map bin/kernel.map \
	-gc-sections

override NASMFLAGS += \
    -Wall \
    -f elf64

override CFILES := $(shell find -L src/* -type f -name '*.c')
override NASMFILES := $(shell find -L src/* -type f -name '*.asm')
override OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(NASMFILES:.asm=.asm.o))
override HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d))
SRC_OBJS = $(CFILES:%=obj/%.o)
ASM_OBJS = $(NASMFILES:%=obj/%.o)
ALL_OBJS = $(SRC_OBJS)
ALL_OBJS += $(ASM_OBJS)

# Add the trampoline object specifically
TRAMPOLINE_OBJ := obj/src/sys/trampoline.asm.o
TRAMPOLINE_BASE := src/sys/trampoline.asm

# Ensure trampoline.asm depends on smp_base.asm being assembled
SMP_BASE_OBJ := obj/src/sys/smp_base.asm.o
SMP_BASE := src/sys/smp_base.asm

# Symbol table from the kernel.map file
SYMBOLS_OUTPUT := include/sys/ksymbols.h
KMAP := bin/kernel.map

# Default target.
.PHONY: all
all: dir include/limine.h bin/$(KERNEL) $(SYMBOLS_OUTPUT)

include/limine.h:
	curl -Lo $@ https://github.com/limine-bootloader/limine/raw/trunk/limine.h

# Kernel linking target
bin/$(KERNEL): linker.ld $(ALL_OBJS)
	@$(LD) $(ALL_OBJS) $(LDFLAGS) -o $@
	@echo "Finished linking"
	@echo "Generating symbols..."
	@../build_helpers/generate_symbols.sh $(KMAP) $(SYMBOLS_OUTPUT)
	@echo "Generated kernel symbol table --> $@"
	@echo "Relinking..."
	@$(LD) $(ALL_OBJS) $(LDFLAGS) -o $@

# Run symbol generation after linking the kernel
.PHONY: post-link
post-link: $(SYMBOLS_OUTPUT)

# Generate ksymbols.h using generate_symbols.sh script after linking
$(SYMBOLS_OUTPUT): $(KMAP)
	@../build_helpers/generate_symbols.sh $(KMAP) $(SYMBOLS_OUTPUT)
	@echo "Generated kernel symbol table --> $@"

-include $(HEADER_DEPS)

# Ensure smp_base.asm is assembled first
$(SMP_BASE_OBJ): src/sys/smp_base.asm
	@nasm $(NASMFLAGS) $< -o $@
	@echo "Assembled --> " $<

# Assemble trampoline.asm after smp_base.asm
$(TRAMPOLINE_OBJ): $(TRAMPOLINE_BASE) $(SMP_BASE_OBJ)
	@nasm $(NASMFLAGS) $< -o $@
	@echo "Assembled --> " $<

obj/%.c.o: %.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(CONDITIONALS) -c $< -o $@
	@echo "Compiled  --> " $<

obj/%.asm.o: %.asm
	@nasm $(NASMFLAGS) $< -o $@
	@echo "Assembled --> " $<

dir:
	@mkdir -p obj/src/sys/gdt
	@mkdir -p obj/src/sys/acpi
	@mkdir -p obj/src/sys/interrupts
	@mkdir -p obj/src/graphics
	@mkdir -p obj/src/sys/tick
	@mkdir -p obj/src/util
	@mkdir -p obj/src/dev/keyboard
	@mkdir -p obj/src/dev/storage
	@mkdir -p obj/src/init
	@mkdir -p obj/src/common
	@mkdir -p obj/src/fs
	@mkdir -p obj/src/proc
	@mkdir -p obj/src/proc/syscalls
	@mkdir -p bin
	@echo "Created obj and bin directories"

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: distclean
distclean: clean
	rm -f include/limine.h
