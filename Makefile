# Nuke built-in rules and variables.
override MAKEFLAGS += -rR
override IMAGE_NAME := horizon

define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

override DEFAULT_HOST_CC := gcc
$(eval $(call DEFAULT_VAR,HOST_CC,$(DEFAULT_HOST_CC)))
override DEFAULT_HOST_CFLAGS := -g -O2 -pipe
$(eval $(call DEFAULT_VAR,HOST_CFLAGS,$(DEFAULT_HOST_CFLAGS)))
override DEFAULT_HOST_CPPFLAGS :=
$(eval $(call DEFAULT_VAR,HOST_CPPFLAGS,$(DEFAULT_HOST_CPPFLAGS)))
override DEFAULT_HOST_LDFLAGS :=
$(eval $(call DEFAULT_VAR,HOST_LDFLAGS,$(DEFAULT_HOST_LDFLAGS)))
override DEFAULT_HOST_LIBS :=
$(eval $(call DEFAULT_VAR,HOST_LIBS,$(DEFAULT_HOST_LIBS)))

.PHONY: all all-hdd run run-uefi run-hdd run-hdd-uefi kernel clean distclean

all: $(IMAGE_NAME).iso

all-hdd: $(IMAGE_NAME).hdd

run: $(IMAGE_NAME).iso
	qemu-system-x86_64 -M q35 -m 2G -debugcon stdio -cdrom $(IMAGE_NAME).iso -boot d

run-uefi: ovmf $(IMAGE_NAME).iso
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf/OVMF.fd -cdrom $(IMAGE_NAME).iso -boot d

run-hdd: $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M q35 -m 2G -debugcon stdio -hda $(IMAGE_NAME).hdd

run-hdd-uefi: ovmf $(IMAGE_NAME).hdd
	qemu-system-x86_64 -M q35 -m 2G -bios ovmf/OVMF.fd -debugcon stdio -hda $(IMAGE_NAME).hdd

ovmf:
	mkdir -p ovmf
	cd ovmf && curl -Lo OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASEX64_OVMF.fd

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
	$(MAKE) -C limine \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

kernel:
	$(MAKE) -C kernel

$(IMAGE_NAME).iso: limine kernel
	@echo "Creating horizon.iso..."
	@rm -rf iso_root
	@mkdir -p iso_root/boot
	@mkdir -p iso_root/boot/limine
	@mkdir -p iso_root/EFI/BOOT
	@cp -v kernel/bin/kernel iso_root/boot/
	@cp -v ext/* iso_root
	@mv iso_root/boot/kernel iso_root/boot/horizon
	@cp -v limine.cfg limine/limine-bios.sys limine/limine-bios-cd.bin \
		limine/limine-uefi-cd.bin iso_root/boot/limine/
	@cp -v limine/BOOTX64.EFI limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	@xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	@./limine/limine bios-install $(IMAGE_NAME).iso
	@rm -rf iso_root

$(IMAGE_NAME).hdd: limine kernel
	@echo "Creating horizon.hdd..."
	@rm -f $(IMAGE_NAME).hdd
	@dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd
	@sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00
	@./limine/limine bios-install $(IMAGE_NAME).hdd
	@mformat -i $(IMAGE_NAME).hdd@@1M
	@mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	@mcopy -i $(IMAGE_NAME).hdd@@1M kernel/bin/kernel ::/boot
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine.cfg limine/limine-bios.sys ::/boot/limine
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT


clean:
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd
	$(MAKE) -C kernel clean

distclean: clean
	rm -rf limine ovmf
	$(MAKE) -C kernel distclean


#
#	Toolchain
#

BINUTILS_VERSION = 2.41
BINUTILS_URL = https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz

GCC_VERSION = 13.2.0
GCC_URL = https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz

TOOLCHAIN_PREFIX = $(abspath ./toolchain/$(TARGET))
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)
export TARGET := x86_64-elf

.PHONY:  toolchain toolchain_binutils toolchain_gcc clean-toolchain clean-toolchain-all

toolchain: toolchain_binutils toolchain_gcc

TOOLCHAIN_DIR = ./toolchain
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

clean-toolchain:
	rm -rf $(GCC_BUILD) $(GCC_SRC) $(BINUTILS_BUILD) $(BINUTILS_SRC)

clean-toolchain-all:
	rm -rf $(TOOLCHAIN_DIR)/*