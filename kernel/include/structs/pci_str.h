/**
 * @file pci_str.h
 * @author Zack Bostock
 * @brief Structures pertaining to the PCI driver 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <stdint.h>

/**
 * @brief Information about a PCI device
 */
typedef struct {
    uint64_t parent;
    uint8_t bus;
    uint8_t func;
    uint8_t device;
} __attribute__((packed)) PCI_DEVICE_INFO;

/**
 * @brief Main PCI device structure
 */
typedef struct {
    PCI_DEVICE_INFO info;
    uint16_t device_id;
    uint16_t vendor_id;
    uint8_t revision_id;
    uint8_t subclass;
    uint8_t device_class;
    uint8_t program_intf;
    uint8_t multifunction;
    uint8_t irq_pin;        // Which pin (INTA#, INTB#, INTC#, INTD#) for int's
    uint8_t has_prt;        // If the device has a PCI routing table
    uint32_t gsi;           // Global system interrupt number
    uint16_t gsi_flags;     // Flags associated with interrupt
} __attribute__((packed)) PCI_DEVICE;

/**
 * @brief Stores information about known PCI devices in a table
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    char description[128];
} PCI_TABLE_ENTRY;


/**
 * @brief Structure to hold data about the Base Address Registers (BAR's)
 */
typedef struct {
    union {
        void *address;
        uint16_t port;
    };
    uint64_t size;
    uint32_t flags;
}   PCI_BAR;