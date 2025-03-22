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

# Directory setup
override LIBCSRC_DIR := ./src
override LIBCOBJ_DIR := ./obj

# Compiler setup
override DEFAULT_CC := ../toolchain/x86_64-elf/bin/x86_64-elf-gcc
$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))

# Common flags for libc functionality
override DEFAULT_CFLAGS := -g 							\
						   -O0							\
						   -Wall						\
						   -Werror						\
						   -pipe						\
						   -lgcc						\
						   -flto						\
						   -ffreestanding 				\
						   -nostdlib 					\
						   -std=gnu11					\
						   -m64 						\
						   -march=x86-64				\
						   -mno-red-zone				\
						   -fpic						\
						   -mgeneral-regs-only			\
						   -mcmodel=large

$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))

override CPPFLAGS := -I . -MMD -MP $(CPPFLAGS)

# Set the source and object files for libc
LIBC_CFILES := $(wildcard $(LIBCSRC_DIR)/*.c)
LIBC_OBJS := $(LIBC_CFILES:$(LIBCSRC_DIR)/%.c=$(LIBCOBJ_DIR)/%.o)

# Create a list of all header dependencies
HEADER_DEPS := $(LIBC_OBJS:.o=.d)

# Default target: build all libc object files
.PHONY: all
all: dir $(LIBC_OBJS)

# Rule to compile each libc source file into an object file
$(LIBCOBJ_DIR)/%.o: $(LIBCSRC_DIR)/%.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "Compiled  --> $<"

# Create necessary directories for object files
dir:
	@mkdir -p $(LIBCOBJ_DIR)
	@echo "Created libc object directory"

# Include header dependencies for incremental builds
-include $(HEADER_DEPS)

# Clean target
.PHONY: clean
clean:
	@rm -rf $(LIBCOBJ_DIR)
	@echo "Cleaned libc object directory"