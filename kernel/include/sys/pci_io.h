/**
 * @file pci_io.h
 * @author Zack Bostock
 * @brief Functions related to PCI Input/Output 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>
#include <sys/pci.h>

#define NOT_IN_TABLE    (-1)

/**
 * @brief (Short) List of known PCI devices
 */
static PCI_TABLE_ENTRY table[] = {
    /* AMD */
    {0x1022, 0x2000, "AMD PCNet Ethernet Controller"},
    {0x1022, 0x1450, "AMD Starship/Matisse Root Complex"},
    {0x1022, 0x43B9, "AMD Ryzen PCI Express Root Complex"},
    {0x1022, 0x7814, "AMD FCH SATA Controller [AHCI mode]"},
    {0x1022, 0x43B1, "AMD PCI Express Root Complex"},
    {0x1022, 0x15D0, "AMD 300 Series Chipset SATA Controller"},
    {0x1022, 0x1439, "AMD Starship USB 3.1 XHCI Controller"},
    {0x1022, 0x1485, "AMD Starship/Matisse PCIe Dummy Function"},
    {0x1022, 0x145A, "AMD Zeppelin SATA Controller [AHCI mode]"},
    {0x1002, 0x67DF, "AMD Radeon RX 580"},
    {0x1002, 0x73BF, "AMD Radeon RX 6900 XT"},
    {0x1002, 0x6860, "AMD Radeon Pro WX 9100"},
    {0x1002, 0x67B1, "AMD Radeon R9 290"},
    {0x1002, 0x6819, "AMD Radeon HD 7850"},
    {0x1002, 0x6613, "AMD Radeon R7 240"},
    {0x1002, 0x687F, "AMD Radeon Vega Frontier Edition"},
    {0x1002, 0x67DF, "AMD Radeon RX 480"},
    {0x1002, 0x73DF, "AMD Radeon RX 6700 XT"},
    {0x1002, 0x7310, "AMD Radeon RX 5700"},
    {0x1002, 0x7340, "AMD Radeon RX 5500M"},
    {0x1002, 0x7341, "AMD Radeon RX 5300M"},
    {0x1002, 0x731F, "AMD Radeon RX 5700 XT"},
    {0x1002, 0x15D8, "AMD Radeon Vega 8 Mobile"},
    {0x1002, 0x1636, "AMD Radeon Vega 10 Mobile"},
    {0x1002, 0x15DD, "AMD Radeon Vega 11 Mobile"},
    {0x1002, 0x1638, "AMD Radeon RX Vega M GL Graphics"},
    {0x1002, 0x15E7, "AMD Radeon Vega 6 Mobile"},

    /* NVIDIA */
    {0x10DE, 0x13C2, "NVIDIA Quadro K620"},
    {0x10DE, 0x1B80, "NVIDIA GeForce GTX 1080"},
    {0x10DE, 0x1C03, "NVIDIA GeForce GTX 1060"},
    {0x10DE, 0x1B06, "NVIDIA GeForce GTX 1070"},
    {0x10DE, 0x1C82, "NVIDIA GeForce GTX 1050 Ti"},
    {0x10DE, 0x1E04, "NVIDIA GeForce RTX 2080"},
    {0x10DE, 0x1F02, "NVIDIA GeForce RTX 2070 Super"},
    {0x10DE, 0x1F07, "NVIDIA GeForce RTX 2060 Super"},
    {0x10DE, 0x1E84, "NVIDIA GeForce RTX 2080 Super"},
    {0x10DE, 0x1F08, "NVIDIA GeForce RTX 2070"},
    {0x10DE, 0x2484, "NVIDIA GeForce GTX 1650"},
    {0x10DE, 0x2206, "NVIDIA GeForce RTX 3080"},
    {0x10DE, 0x2504, "NVIDIA GeForce RTX 3060 Ti"},
    {0x10DE, 0x2489, "NVIDIA GeForce RTX 3050"},
    {0x10DE, 0x2610, "NVIDIA GeForce RTX 4090"},
    {0x10DE, 0x2684, "NVIDIA GeForce RTX 4080"},
    {0x10DE, 0x26C0, "NVIDIA GeForce RTX 4070 Ti"},
    {0x10DE, 0x2784, "NVIDIA GeForce RTX 4060"},
    {0x10DE, 0x1C60, "NVIDIA GeForce GTX 1050 Mobile"},
    {0x10DE, 0x1C20, "NVIDIA GeForce GTX 1060 Mobile"},
    {0x10DE, 0x1BA1, "NVIDIA GeForce GTX 1070 Mobile"},
    {0x10DE, 0x1C81, "NVIDIA GeForce GTX 1050 Ti Mobile"},
    {0x10DE, 0x1E90, "NVIDIA GeForce RTX 2060 Mobile"},
    {0x10DE, 0x1F10, "NVIDIA GeForce RTX 2070 Mobile"},
    {0x10DE, 0x1EAB, "NVIDIA GeForce RTX 2080 Mobile"},
    {0x10DE, 0x2520, "NVIDIA GeForce RTX 3060 Mobile"},
    {0x10DE, 0x24DD, "NVIDIA GeForce RTX 3070 Mobile"},
    {0x10DE, 0x2525, "NVIDIA GeForce RTX 3080 Mobile"},

    /* Realtek */
    {0x10EC, 0x8139, "Realtek RTL8139 Fast Ethernet Adapter"},
    {0x10EC, 0x8168, "Realtek PCIe Gigabit Ethernet Family Controller"},
    {0x10EC, 0x5229, "Realtek PCIe GBE Family Controller"},

    /* VIA */
    {0x1106, 0x3044, "VIA VT6306 Fire II IEEE 1394 Host Controller"},

    /* ASPEED */
    {0x1A03, 0x2000, "ASPEED Graphics Family"},

    /* ASMedia */
    {0x1B21, 0x0612, "ASMedia ASM1062 Serial ATA Controller"},

    /* Broadcom */
    {0x14E4, 0x1693, "Broadcom NetXtreme BCM5751 Gigabit Ethernet"},

    /* Qualcomm Atheros */
    {0x168C, 0x0030, "Qualcomm Atheros AR93xx Wireless Network Adapter"},

    /* VirtualBox */
    {0x80EE, 0xBEEF, "VirtualBox Graphics Adapter"},
    {0x80EE, 0xCAFE, "VirtualBox Guest Service"},
    {0x80EE, 0xCACA, "VirtualBox AC97 Audio Controller"},
    {0x80EE, 0xDEAD, "VirtualBox Ethernet Adapter"},
    {0x80EE, 0xBEEB, "VirtualBox USB Tablet"},

    /* QEMU */
    {0x1234, 0x1111, "QEMU Standard VGA Display Adapter"},
    {0x1234, 0x1110, "QEMU PCIe Root Port"},
    {0x1AF4, 0x1000, "QEMU Virtio Network Device"},
    {0x1AF4, 0x1001, "QEMU Virtio Block Device"},
    {0x1AF4, 0x1002, "QEMU Virtio SCSI Device"},
    {0x1AF4, 0x1003, "QEMU Virtio Memory Balloon"},
    {0x1AF4, 0x1004, "QEMU Virtio Console"},

    /* Intel */
    {0x8086, 0x0154, "3rd Gen Core processor DRAM Controller"},
    {0x8086, 0x100E, "Intel PRO/1000 MT Network Connection"},
    {0x8086, 0x1C20, "Intel 6 Series/C200 Series Chipset Family PCI Express Root Port 1"},
    {0x8086, 0x1D3B, "Intel C600/X79 Series Chipset PCI Express Root Port 2"},
    {0x8086, 0x24D3, "Intel 82801EB/ER (ICH5/ICH5R) USB2 Enhanced Host Controller"},
    {0x8086, 0x2918, "Intel ICH9 Family SMBus Controller"},
    {0x8086, 0x3B42, "Intel 5 Series/3400 Series Chipset PCI Express Root Port 1"},
    {0x8086, 0x3B64, "Intel 5 Series/3400 Series Chipset USB2 Enhanced Host Controller"},
    {0x8086, 0x9D14, "Intel Sunrise Point-LP PCI Express Root Port #1"},
    {0x8086, 0x0C01, "Intel Xeon E3-1200 v3/4th Gen Core Processor PCI Express Root Port"},
    {0x8086, 0x1503, "Intel 82579LM Gigabit Network Connection"},
    {0x8086, 0x153A, "Intel I210 Gigabit Network Connection"},
    {0x8086, 0x10D3, "Intel 82574L Gigabit Network Connection"},
    {0x8086, 0x0F00, "Intel Atom Processor Z36xxx/Z37xxx Series PCI Express Root Port"},
    {0x8086, 0x10C9, "Intel 82576 Gigabit Network Connection"},
    {0x8086, 0x1F3C, "Intel Atom Processor C2000 PCI Express Root Port"},
    {0x8086, 0x8D02, "Intel C610/X99 Series Chipset PCI Express Root Port #3"},
    {0x8086, 0x1F40, "Intel Atom Processor C2000 QuickAssist Technology"},
    {0x8086, 0x8C31, "Intel 8 Series/C220 Series Chipset Family SATA AHCI Controller"},
    {0x8086, 0x8C22, "Intel 8 Series/C220 Series Chipset Family USB Enhanced Host Controller"},
    {0x8086, 0x8C10, "Intel 8 Series/C220 Series Chipset Family PCI Express Root Port #1"},
    {0x8086, 0x8C16, "Intel 8 Series/C220 Series Chipset Family PCI Express Root Port #5"},
    {0x8086, 0x8C50, "Intel 8 Series/C220 Series Management Engine Interface"},
    {0x8086, 0x1533, "Intel I217-V Gigabit Network Connection"},
    {0x8086, 0x0A16, "Intel HD Graphics 4600"},
    {0x8086, 0x0C04, "Intel Haswell PCI Express x8 Controller"},
    {0x8086, 0x0F31, "Intel Atom Processor Z3700 Series PCI Express Root Port"},
    {0x8086, 0x29C0, "82G33/G31/P35/P31 Express DRAM Controller"},
    {0x8086, 0x2922, "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]"},
    {0x8086, 0x2930, "82801I (ICH9 Family) SMBus Controller"}
};

/**
 * @brief PCI version of the ind instruction
 * 
 * @param bus Selected bus to read from
 * @param slot Selected slot to read from 
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @return uint32_t Data read from the device 
 */
static inline uint32_t pci_ind(uint32_t bus, uint32_t slot, uint32_t func,
                               uint8_t offset) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    return ind(PCI_CONFIG_DATA);
}

/**
 * @brief PCI version of the outd instruction
 * 
 * @param bus Selected bus to write to 
 * @param slot Selected slot to write to
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @param data Data to write to device
 */
static inline void pci_outd(uint32_t bus, uint32_t slot, uint32_t func,
                            uint8_t offset, uint32_t data) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    outd(PCI_CONFIG_DATA, data);
}

/**
 * @brief PCI version of the inw instruction
 * 
 * @param bus Selected bus to read from
 * @param slot Selected slot to read from 
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @return uint16_t Data read from the device 
 */
static inline uint16_t pci_inw(uint32_t bus, uint32_t slot, uint32_t func,
                               uint8_t offset) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    return inw(PCI_CONFIG_DATA + (offset & 0x02));
}

/**
 * @brief PCI version of the outw instruction
 * 
 * @param bus Selected bus to write to 
 * @param slot Selected slot to write to
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @param data Data to write to device
 */
static inline void pci_outw(uint32_t bus, uint32_t slot, uint32_t func,
                            uint8_t offset, uint32_t data) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    outw(PCI_CONFIG_DATA  + (offset & 0x02), data);
}

/**
 * @brief PCI version of the inb instruction
 * 
 * @param bus Selected bus to read from
 * @param slot Selected slot to read from 
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @return uint8_t Data read from the device 
 */
static inline uint8_t pci_inb(uint32_t bus, uint32_t slot, uint32_t func,
                               uint8_t offset) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    return inb(PCI_CONFIG_DATA + (offset & 0x03));
}

/**
 * @brief PCI version of the outb instruction
 * 
 * @param bus Selected bus to write to 
 * @param slot Selected slot to write to
 * @param func Selected function of the device 
 * @param offset Offset into the register 
 * @param data Data to write to device
 */
static inline void pci_outb(uint32_t bus, uint32_t slot, uint32_t func,
                            uint8_t offset, uint32_t data) {
    uint32_t addr = (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) |
                    0x80000000;
    outd(PCI_CONFIG_ADDR, addr);
    outb(PCI_CONFIG_DATA  + (offset & 0x03), data);
}

/**
 * @brief Helper macro to check if a function exists for a specific device
 * @verbatim
 * When checking if a function exists, if the readback does not come back
 * as 0xFFFF, then the function does not exist.
 */
#define PCI_FUNCTION_EXISTS(bus, slot, func)                                \
    ((uint16_t)(pci_ind(bus, slot, func, 0x0) & 0xFFFF) != 0xFFFF)

/**
 * @brief Helper macro for finding the vendor ID of a device
 */
#define PCI_READ_VENDOR_ID(bus, slot, func)                                 \
    (pci_ind(bus, slot, func, 0x0) & 0xFFFF)

/**
 * @brief Helper macro for finding the device ID of a device
 */
#define PCI_READ_DEVICE_ID(bus, slot, func)                                 \
    ((pci_ind(bus, slot, func, 0x0) >> 16) & 0xFFFF)

/**
 * @brief Reads the header type of a specific device
 */
#define PCI_READ_HEADER(bus, slot, func)                                    \
    ((pci_ind(bus, slot, func, PCI_CLASS_SERIAL_BUS) >> 16) & 0xFF)

/**
 * @brief Reads the class type of a specific device
 */
#define PCI_READ_CLASS(bus, slot, func)                                     \
    ((pci_ind(bus, slot, func, PCI_CLASS_SYSTEM_PERIPHERAL)) >> 16)

/**
 * @brief Reads the subclass type of a specific device
 */
#define PCI_READ_SUBCLASS(bus, slot, func)                                  \
    (((pci_ind(bus, slot, func, PCI_CLASS_SYSTEM_PERIPHERAL)) >> 16) & 0xFF)

/**
 * @brief Checks if a device is a bridge (PCI-to-PCI) device
 */
#define PCI_DEV_IS_BRIDGE(bus, slot, func)                                  \
    (PCI_READ_HEADER(bus, slot, func) == 0x1 &&                             \
    PCI_READ_CLASS(bus, slot, func) == PCI_CLASS_BRIDGE_DEVICE)

/**
 * @brief Reads the Subordinate Bus Number from a device
 */
#define PCI_READ_SUBBUS_ID(bus, slot, func)                                 \
    ((pci_ind(bus, slot, func, 0x18) >> 24) && 0xFF)

/**
 * @brief Macro to read the Program Interface from a device
 */
#define PCI_READ_PROGRAM_INTERFACE(bus, slot, func)                         \
    ((pci_ind(bus, slot, func, PCI_CLASS_SYSTEM_PERIPHERAL) >> 8) & 0xFF)

/**
 * @brief Macro to check if the device is multifunction
 */
#define PCI_IS_DEV_MULTIFUNCTION(bus, slot, func)                           \
    ((pci_ind(bus, slot, func, PCI_CLASS_SERIAL_BUS) >> 16) & 0x80)

#define PCI_DEV_INFO(dev, dev_number) {                                     \
    if (dev_number == NOT_IN_TABLE) {                                       \
        klogi("PCI: %2x:%2x.%1x - %4x:%4x %s\n",                            \
              dev.info.bus, dev.info.device, dev.info.func, dev.vendor_id,  \
              dev.device_id, "Unknown Device");                             \
    } else {                                                                \
        klogi("PCI: %2x:%2x.%1x - %4x:%4x %s\n",                            \
              dev.info.bus, dev.info.device, dev.info.func, dev.vendor_id,  \
              dev.device_id, table[dev_number].description);                \
    }                                                                       \
}
