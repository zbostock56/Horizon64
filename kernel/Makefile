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
override DEFAULT_CFLAGS := -g -O2 -pipe -lgcc
$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))
override DEFAULT_CPPFLAGS :=
$(eval $(call DEFAULT_VAR,CPPFLAGS,$(DEFAULT_CPPFLAGS)))
override DEFAULT_NASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,NASMFLAGS,$(DEFAULT_NASMFLAGS)))
override DEFAULT_LDFLAGS :=
$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

override CFLAGS += \
    -Wall \
	-Werror \
	-ffreestanding \
	-nostdlib \
    -fPIC \
    -std=gnu11 \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fPIE \
		-fno-omit-frame-pointer \
    -m64 \
    -march=x86-64 \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=large \
    -mno-red-zone

override CPPFLAGS := \
    -I ./include \
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
		-Map bin/kernel.map

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

# Default target.
.PHONY: all
all: dir include/limine.h bin/$(KERNEL)

include/limine.h:
	curl -Lo $@ https://github.com/limine-bootloader/limine/raw/trunk/limine.h

bin/$(KERNEL): linker.ld $(ALL_OBJS)
	@$(LD) $(ALL_OBJS) $(LDFLAGS) -o $@
	@echo "Finished linking"

-include $(HEADER_DEPS)

obj/%.c.o: %.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "Compiled --> " $<

obj/%.asm.o: %.asm
	@nasm $(NASMFLAGS) $< -o $@
	@echo "Assembled --> " $<

dir:
	@mkdir -p obj/src/sys/gdt
	@mkdir -p obj/src/sys/interrupts
	@mkdir -p obj/src/graphics
	@mkdir -p obj/src/sys/tick
	@mkdir -p obj/src/util
	@mkdir -p obj/src/dev/keyboard
	@mkdir -p obj/src/init
	@mkdir -p obj/src/common
	@mkdir -p bin
	@echo "Created obj and bin directories"

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: distclean
distclean: clean
	rm -f include/limine.h
