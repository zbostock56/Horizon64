# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# List of executables to be compiled
EXECUTABLES := echo cat hsh init

# Directory setup
override USERSRC_DIR := src
override USERINC_DIR := include -I ../h64libc
override USERBIN_DIR := bin
LIBC_OBJ_DIR := ../h64libc/obj

# Compiler and assembler setup
override DEFAULT_CC := ../toolchain/x86_64-elf/bin/x86_64-elf-gcc
$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))
override DEFAULT_ASM := nasm

# Common flags for userspace programs
override DEFAULT_CFLAGS := -g 							\
						   -O0							\
                           -Wall                        \
                           -Werror                      \
						   -pipe						\
						   -lgcc						\
						   -flto						\
						   -ffreestanding 				\
						   -nostdlib 					\
						   -std=gnu11					\
						   -m64 						\
						   -march=x86-64				\
						   -mno-red-zone				\
						   -fno-pic						\
						   -mgeneral-regs-only			\
						   -mcmodel=large

$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))
override DEFAULT_LDFLAGS := -static						\
							-flto						\
							-no-pie 					\
							-fno-builtin				\
							-nostdlib					\
							-T linker.ld				\
							-z max-page-size=0x1000		\
							-I .						\
							-O0							\
							-mcmodel=large -g

$(eval $(call DEFAULT_VAR,LDFLAGS,$(DEFAULT_LDFLAGS)))

# Assembly flags
override NASMFLAGS := -f elf64 -g -F dwarf

# CPPFLAGS: Include paths for headers from both ./include and ../h64libc
override CPPFLAGS := -I $(USERINC_DIR) -MMD -MP $(CPPFLAGS)

# Set the source and object files for each executable
USER_CFILES := $(foreach exec,$(EXECUTABLES),$(USERSRC_DIR)/$(exec).c)
USER_ASMFILES := $(wildcard $(USERSRC_DIR)/*.asm)
USER_OBJS := $(addprefix obj/,$(notdir $(USER_CFILES:.c=.o)) $(notdir $(USER_ASMFILES:.asm=.o)))

# Collect all libc object files
LIBC_OBJS := $(wildcard $(LIBC_OBJ_DIR)/*.o)

# Create a list of all header dependencies
HEADER_DEPS := $(USER_OBJS:.o=.d)

# Default target: build all executables
.PHONY: all
all: dir $(USER_OBJS) $(EXECUTABLES)
	@echo "Moving userspace binaries into initrd/bin..."
	@mkdir -p ../initrd/bin
	@cp $(wildcard bin/*) ../initrd/bin

# Rule to build each executable
# Manually specify the name of the executable using $< and also include crt0.o
# to build each individual user space executable
$(EXECUTABLES): %: obj/%.o
	@$(CC) $< obj/crt0.o $(LIBC_OBJS) $(LDFLAGS) -o $(USERBIN_DIR)/$@
	@echo "Created binary --> $@"

# Rule to compile the C files into object files
obj/%.o: $(USERSRC_DIR)/%.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "Compiled --> $<"

# Rule to assemble the .asm files into object files
obj/crt0.o: $(USERSRC_DIR)/crt0.asm
	@$(DEFAULT_ASM) $(NASMFLAGS) $< -o $@
	@echo "Assembled --> $<"

# Create necessary directories for object and binary files
dir:
	@mkdir -p obj bin
	@echo "Created directories for obj and bin"

# Include header dependencies for incremental builds
-include $(HEADER_DEPS)

# Clean target
.PHONY: clean
clean:
	@rm -rf bin obj
	@echo "Cleaned userspace object and binary directories"
