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

# Include necessary headers
override INC_DIR := -I . -I libc -I src

# Compiler setup
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
override DEFAULT_CC := x86_64-elf-gcc
else
override DEFAULT_CC := ../toolchain/x86_64-elf/bin/x86_64-elf-gcc
endif
$(eval $(call DEFAULT_VAR,CC,$(DEFAULT_CC)))

# Common flags for libc functionality
override DEFAULT_CFLAGS := -g 							\
						   -O0							\
						   -Wall						\
						   -Werror						\
						   -pipe						\
						   -lgcc						\
						   -fno-lto						\
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

override CPPFLAGS := $(INC_DIR) -MMD -MP $(CPPFLAGS)

# Set the source and object files for libc
LIBC_CFILES := $(shell find $(LIBCSRC_DIR) -name '*.c')
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
	@find $(LIBCSRC_DIR) -type d | sed "s|^$(LIBCSRC_DIR)|$(LIBCOBJ_DIR)|" | xargs -I{} mkdir -p {}
	@echo "Created libc object directory structure"

# Include header dependencies for incremental builds
-include $(HEADER_DEPS)

# Clean target
.PHONY: clean
clean:
	@rm -rf $(LIBCOBJ_DIR)
	@echo "Cleaned libc object directory"