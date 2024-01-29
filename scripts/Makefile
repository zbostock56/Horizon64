BINUTILS_VERSION = 2.41
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 13.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz

TOOLCHAIN_PREFIX = $(abspath ../toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)
export TARGET := i686-elf

toolchain: toolchain_binutils toolchain_gcc

TOOLCHAIN_DIR = ../toolchain
BINUTILS_SRC = $(TOOLCHAIN_DIR)/binutils-$(BINUTILS_VERSION)
BINUTILS_BUILD = $(TOOLCHAIN_DIR)/binutils-build-$(BINUTILS_VERSION)

toolchain_binutils: $(BINUTILS_SRC).tar.xz
	cd toolchain && tar -xf binutils-$(BINUTILS_VERSION).tar.xz
	mkdir $(BINUTILS_BUILD)
	cd $(BINUTILS_BUILD) && ../binutils-$(BINUTILS_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)"	\
		--target=$(TARGET)				\
		--with-sysroot					\
		--disable-nls					\
		--disable-werror
	$(MAKE) -j4 -C $(BINUTILS_BUILD)
	$(MAKE) -C $(BINUTILS_BUILD) install

$(BINUTILS_SRC).tar.xz:
	mkdir -p $(TOOLCHAIN_DIR) 
	cd $(TOOLCHAIN_DIR) && wget $(BINUTILS_URL)


GCC_SRC = $(TOOLCHAIN_DIR)/gcc-$(GCC_VERSION)
GCC_BUILD = $(TOOLCHAIN_DIR)/gcc-build-$(GCC_VERSION)

toolchain_gcc: toolchain_binutils $(GCC_SRC).tar.xz
	cd $(TOOLCHAIN_DIR) && tar -xf gcc-$(GCC_VERSION).tar.xz
	mkdir $(GCC_BUILD)
	cd $(GCC_BUILD) && ../gcc-$(GCC_VERSION)/configure \
		--prefix="$(TOOLCHAIN_PREFIX)" 	\
		--target=$(TARGET)				\
		--disable-nls					\
		--enable-languages=c,c++		\
		--without-headers
	$(MAKE) -j4 -C $(GCC_BUILD) all-gcc all-target-libgcc
	$(MAKE) -C $(GCC_BUILD) install-gcc install-target-libgcc
	
$(GCC_SRC).tar.xz:
	mkdir -p $(TOOLCHAIN_DIR)
	cd $(TOOLCHAIN_DIR) && wget $(GCC_URL)

#
# Clean
#
clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf $(TOOLCHAIN_DIR)/*

.PHONY:  toolchain toolchain_binutils toolchain_gcc clean-toolchain clean-toolchain-all