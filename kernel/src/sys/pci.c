/**
 * @file pci.c
 * @author Zack Bostock
 * @brief Functionality pertaining to (Conventional) PCI driver 
 * @verbatim
 * Peripheral Component Interconnect (PCI) is a local computer bus for
 * attaching hardware devices in a computer and is part of the PCI Local
 * Bus standard.
 * @ref https://wiki.osdev.org/PCI
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <sys/asm.h>
#include <sys/pci.h>
#include <sys/pci_io.h>
#include <common/vector.h>
#include <common/kprint.h>

vector_new(PCI_DEVICE, device_list);

/**
 * @brief Helper function for finding a description on found PCI devices
 * 
 * @param device Device to find description for
 * @return const char * Description for device
 */
uint64_t find_device_desc(PCI_DEVICE *device) {
    uint64_t size = sizeof(table) / sizeof(PCI_TABLE_ENTRY);
    for (uint64_t i = 0; i < size; i++) {
        if (table[i].vendor_id == device->vendor_id &&
            table[i].device_id == device->device_id) {
            return i;
        }
    }
    return NOT_IN_TABLE;
}

/**
 * @brief Helper function to find specifics about mask and address of BAR
 * 
 * @param bus Bus which BAR is on 
 * @param slot Slot which BAR is on
 * @param func Function chosen
 * @param bar Bar to inspect
 * @param address Passed in to fulfill
 * @param mask Passed in to fulfill
 */
inline static void read_bar(uint32_t bus, uint32_t slot, uint32_t func,
                     uint8_t bar, uint32_t *address, uint32_t *mask) {
    /* Find the address */
    *address = pci_ind(bus, slot, func, bar);

    /* Find size of bar */
    pci_outd(bus, slot, func, bar, 0xFFFFFFFF);
    *mask = pci_ind(bus, slot, func, bar);

    /* Restore the address of the BAR */
    pci_outd(bus, slot, func, bar, *address);
}

/**
 * @brief Helper to find fill PCI_BAR structure about a specific BAR
 * 
 * @param bus Bus which the BAR is on 
 * @param slot Slot which the BAR is on
 * @param func Function chosen
 * @param bar Bar to inspect
 * @return PCI_BAR Structure of information about BAR
 */
PCI_BAR find_bar(uint32_t bus, uint32_t slot, uint32_t func, uint8_t bar) {
    uint32_t addr_low;
    uint32_t mask_low;
    read_bar(bus, slot, func, bar, &addr_low, &mask_low);

    if (addr_low & PCI_BAR_TYPE_64BIT) {
        /* 64-bit Memory Mapped I/O */
        uint32_t addr_high;
        uint32_t mask_high;
        read_bar(bus, slot, func, bar, &addr_high, &mask_high);
        PCI_BAR b = {
            .address = (void *) (((uintptr_t) addr_high << 32) | (addr_low & PCI_BAR_MEM_MASK)),
            .size = (((uint64_t) mask_high << 32) | (mask_low & PCI_BAR_MEM_MASK)) + 1,
            .flags = addr_low & 0xF
        };
        return b;
    } else if (addr_low & PCI_BAR_IO) {
        /* I/O register */
        PCI_BAR b = {
            .port = (uint16_t)(addr_low & (PCI_BAR_MEM_MASK | 0xC)),
            .size = (uint16_t)(~(mask_low & (PCI_BAR_MEM_MASK | 0xC)) + 1),
            .flags = addr_low & 0x3
        };
        return b;
    } else {
        /* 32-bit Memory Mapped I/O */
        PCI_BAR b = {
            .address = (void *)(uintptr_t)(addr_low & PCI_BAR_MEM_MASK),
            .size = ~(mask_low & PCI_BAR_MEM_MASK) + 1,
            .flags = addr_low & 0xF
        };
        return b;
    }
}

/**
 * @brief Helper function to scan a device (and more connected to it if
 *        applicable)
 * 
 * @param bus Bus of device to scan
 * @param device_id Device ID of device to scan 
 */
static void scan_device(uint8_t bus, uint8_t device_id) {
    PCI_DEVICE dev = {0};
    dev.info.bus = bus;
    dev.info.func = 0;
    dev.info.device = device_id;

    uint8_t function_exists = PCI_FUNCTION_EXISTS(dev.info.bus, dev.info.device,
                                                  dev.info.func);
    uint8_t is_bridge = PCI_DEV_IS_BRIDGE(dev.info.bus, dev.info.device,
                                          dev.info.func);
    uint8_t has_multi_function = PCI_IS_DEV_MULTIFUNCTION(dev.info.bus,
                                                          dev.info.device,
                                                          dev.info.func);

    if (is_bridge) {
        klogi("PCI:%x:%x.%x - %x:%x [bridge] function %s\n",
              dev.info.bus, dev.info.device, dev.info.func, dev.vendor_id,
              dev.device_id, function_exists ? "existed" : "didn't exist");
    }

    if (function_exists) {
        if (is_bridge) {
            uint8_t sub_bus_id = PCI_READ_SUBBUS_ID(dev.info.bus, dev.info.device,
                                                    dev.info.func);
            if (sub_bus_id != bus) {
                klogi("PCI: Read sub bus %x\n", sub_bus_id);
                pci_scan_bus(PCI_READ_SUBBUS_ID(dev.info.bus, dev.info.device,
                                                dev.info.func));
            }
        }

        dev.multifunction = has_multi_function;
        dev.device_id = PCI_READ_DEVICE_ID(dev.info.bus, dev.info.device,
                                           dev.info.func);
        dev.vendor_id = PCI_READ_VENDOR_ID(dev.info.bus, dev.info.device,
                                           dev.info.func);
        PCI_DEV_INFO(dev, find_device_desc(&dev));
        vector_append(&device_list, dev);

        if (dev.multifunction) {
            for (uint8_t i = 1; i < MAX_FUNCTION_NUM; i++) {
                PCI_DEVICE dev_2 = {0};
                dev_2.info.bus = bus;
                dev_2.info.func = i;
                dev_2.info.device = device_id;

                if (PCI_FUNCTION_EXISTS(dev_2.info.bus, dev_2.info.device,
                                        dev_2.info.func)) {
                    dev_2.device_id = PCI_READ_DEVICE_ID(dev_2.info.bus,
                                                         dev_2.info.device,
                                                         dev_2.info.func);

                    dev_2.vendor_id = PCI_READ_VENDOR_ID(dev_2.info.bus,
                                                         dev_2.info.device,
                                                         dev_2.info.func);
                    PCI_DEV_INFO(dev_2, find_device_desc(&dev_2));
                    vector_append(&device_list, dev_2);
                }
            }
        }
    }
}

/**
 * @brief Helper function to scan for all devices on a bus
 * 
 * @param bus Bus to scan 
 */
void pci_scan_bus(uint8_t bus) {
    for (size_t i = 0; i < MAX_DEVICE_NUM; i++) {
        scan_device(bus, i);
    }
}

/**
 * @brief Helper function to list all devices that were found at boot
 */
void pci_list() {
    for (size_t i = 0; i < vector_len(&device_list); i++) {
        PCI_DEVICE dev = vector_at(&device_list, i);
        PCI_DEV_INFO(dev, find_device_desc(&dev));
    }
}

/**
 * @brief Main PCI initialization function
 * @verbatim
 * Here, go through all the buses and scan for all possible devices. While
 * scanning a device, we check to see if there are any bridges between that
 * device and another to continue scanning that bridge.
 */
void pci_init() {
    klogi("INIT PCI: starting...\n");
    for (size_t bus_id = 0; bus_id < MAX_BUS_NUM; bus_id++) {
        for (size_t device = 0; device < MAX_DEVICE_NUM; device++) {
            scan_device(bus_id, device);
        }
    }

    klogi("PCI: Device scan completed with (%d) devices found\n",
          vector_len(&device_list));
    klogi("INIT PCI: finished...\n");
}