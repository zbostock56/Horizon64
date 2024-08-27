/**
 * @file pci.h
 * @author Zack Bostock
 * @brief Information pertaining to the (Conventional) PCI driver 
 * @verbatim
 * Peripheral Component Interconnect (PCI) is a local computer bus for
 * attaching hardware devices in a computer and is part of the PCI Local
 * Bus standard.
 * @ref https://wiki.osdev.org/PCI
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <structs/pci_str.h>

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
/**
 * @brief General constants around PCI standard
 */
#define MAX_BUS_NUM                          (1)
#define MAX_DEVICE_NUM                       (32)
#define MAX_FUNCTION_NUM                     (8)

/**
 * @brief Literal constants pertaining to PCI I/O ports
 */
#define PCI_CONFIG_ADDR                      (0xCF8)
#define PCI_CONFIG_DATA                      (0xCFC)
#define FORWARDING_REG                       (0xCFA)

/**
 * @brief Literal constants pertaining to the header types of the PCI devices
 */
#define PCI_GENERIC                          (0x00)
#define PCI_PCI_BRIDGE                       (0x01)
#define PCI_CARDBUS_BRIDGE                   (0x02)
#define PCI_MULTI_FUNCTION                   (0x80)

/**
 * @brief Literal constants pertaining to the common PCI vendors
 */
#define PCI_VENDOR_ID_INTEL                  (0x8086)
#define PCI_VENDOR_ID_AMD                    (0x1022)
#define PCI_VENDOR_ID_NVIDIA                 (0x10DE)
#define PCI_VENDOR_ID_BROADCOM               (0x14E4)
#define PCI_VENDOR_ID_QUALCOMM               (0x17CB)
#define PCI_VENDOR_ID_ATHEROS                (0x168C)
#define PCI_VENDOR_ID_REALTEK                (0x10EC)
#define PCI_VENDOR_ID_MARVELL                (0x1B4B)
#define PCI_VENDOR_ID_DELL                   (0x1028)
#define PCI_VENDOR_ID_HP                     (0x103C)
#define PCI_VENDOR_ID_ASMEDIA                (0x1B21)
#define PCI_VENDOR_ID_LENOVO                 (0x17AA)
#define PCI_VENDOR_ID_VIA                    (0x1106)
#define PCI_VENDOR_ID_TEXAS_INSTRUMENTS      (0x104C)
#define PCI_VENDOR_ID_MATROX                 (0x102B)

/**
 * @brief PCI Configuration Registers
 */
#define PCI_VENDOR_ID                        (0x00)  // 16 bits, Vendor ID
#define PCI_DEVICE_ID                        (0x02)  // 16 bits, Device ID
#define PCI_COMMAND                          (0x04)  // 16 bits, Command register
#define PCI_STATUS                           (0x06)  // 16 bits, Status register
#define PCI_REVISION_ID                      (0x08)  // 8 bits, Revision ID
#define PCI_PROG_IF                          (0x09)  // 8 bits, Programming Interface
#define PCI_SUBCLASS                         (0x0A)  // 8 bits, Subclass code
#define PCI_CLASS_CODE                       (0x0B)  // 8 bits, Class code
#define PCI_CACHE_LINE_SIZE                  (0x0C)  // 8 bits, Cache Line Size
#define PCI_LATENCY_TIMER                    (0x0D)  // 8 bits, Latency Timer
#define PCI_HEADER_TYPE                      (0x0E)  // 8 bits, Header Type
#define PCI_BIST                             (0x0F)  // 8 bits, Built-in Self Test (BIST)

/**
 * @brief List of Generic Configuration Registers
 */
#define PCI_BAR0                             (0x10)  // 32 bits, Base Address Register 0
#define PCI_BAR1                             (0x14)  // 32 bits, Base Address Register 1
#define PCI_BAR2                             (0x18)  // 32 bits, Base Address Register 2
#define PCI_BAR3                             (0x1C)  // 32 bits, Base Address Register 3
#define PCI_BAR4                             (0x20)  // 32 bits, Base Address Register 4
#define PCI_BAR5                             (0x24)  // 32 bits, Base Address Register 5
#define PCI_CARDBUS_CIS                      (0x28)  // 32 bits, Cardbus CIS Pointer
#define PCI_SUBSYSTEM_VENDOR_ID              (0x2C)  // 16 bits, Subsystem Vendor ID
#define PCI_SUBSYSTEM_ID                     (0x2E)  // 16 bits, Subsystem ID
#define PCI_EXPANSION_ROM_BASE_ADDR          (0x30)  // 32 bits, Expansion ROM base address
#define PCI_CAPABILITY_LIST                  (0x34)  // 8 bits, Capabilities Pointer
#define PCI_INTERRUPT_LINE                   (0x3C)  // 8 bits, Interrupt Line
#define PCI_INTERRUPT_PIN                    (0x3D)  // 8 bits, Interrupt Pin
#define PCI_MIN_GRANT                        (0x3E)  // 8 bits, Minimum Grant
#define PCI_MAX_LATENCY                      (0x3F)  // 8 bits, Maximum Latency

/**
 * @brief Masks for BAR values
 */
#define PCI_BAR_IO_MASK                      (0x00000001)  // Indicates I/O space
#define PCI_BAR_MEM_MASK                     (0xFFFFFFF0)  // For memory space mask
#define PCI_BAR_TYPE_MASK                    (0x00000006)  // Memory type (32-bit or 64-bit)
#define PCI_BAR_PREFETCHABLE_MASK            (0x00000008)  // Prefetchable bit

/**
 * @brief Values for memory type in BAR
 */
#define PCI_BAR_TYPE_32BIT                   (0x00000000)  // 32-bit address space
#define PCI_BAR_TYPE_64BIT                   (0x00000004)  // 64-bit address space

/**
 * @brief I/O Space BAR
 */
#define PCI_BAR_IO                           (0x00000001)

/**
 * @brief Memory Space BAR
 */
#define PCI_BAR_MEM                          (0x00000000)  // Memory space indicator
#define PCI_BAR_MEM_PREFETCHABLE             (0x00000008)  // Memory space is prefetchable

/**
 * @brief Literal constants pertaining to the common mass storage device
 */
#define PCI_CLASS_MASS_STORAGE               (0x01)
#define PCI_STORAGE_SCSI                     (0x00)
#define PCI_STORAGE_IDE                      (0x01)
#define PCI_STORAGE_FLOPPY_DISK              (0x02)
#define PCI_STORAGE_IPI_BUS                  (0x03)
#define PCI_STORAGE_RAID                     (0x04)
#define PCI_STORAGE_ATA                      (0x05)  // Includes parallel ATA
#define PCI_STORAGE_SATA                     (0x06)  // Serial ATA
#define PCI_STORAGE_SAS                      (0x07)  // Serial Attached SCSI
#define PCI_STORAGE_NVM                      (0x08)  // Non-Volatile Memory (NVMe)
#define PCI_STORAGE_MASS_STORAGE_OTHER       (0x80)

/**
 * @brief Literal constants pertaining to the common network controllers
 */
#define PCI_CLASS_NETWORK_CONTROLLER         (0x02)
#define PCI_NETWORK_ETHERNET                 (0x00)
#define PCI_NETWORK_TOKEN_RING               (0x01)
#define PCI_NETWORK_FDDI                     (0x02)
#define PCI_NETWORK_ATM                      (0x03)
#define PCI_NETWORK_ISDN                     (0x04)
#define PCI_NETWORK_WORLDFIP                 (0x05)
#define PCI_NETWORK_PICMG                    (0x06)  // PICMG 2.14 Multi Computing
#define PCI_NETWORK_INFINIBAND               (0x07)
#define PCI_NETWORK_FABRIC                   (0x08)
#define PCI_NETWORK_NETWORK_OTHER            (0x80)

/**
 * @brief Literal constants pertaining to common display controllers
 */
#define PCI_CLASS_DISPLAY_CONTROLLER         (0x03)
#define PCI_DISPLAY_VGA_COMPATIBLE           (0x00)
#define PCI_DISPLAY_XGA                      (0x01)
#define PCI_DISPLAY_3D                       (0x02)  // Non-VGA-compatible 3D controller
#define PCI_DISPLAY_DISPLAY_OTHER            (0x80)

/**
 * @brief Literal constants pertaining to a multimedia controllers
 */
#define PCI_CLASS_MULTIMEDIA                 (0x04)
#define PCI_MULTIMEDIA_VIDEO                 (0x00)
#define PCI_MULTIMEDIA_AUDIO                 (0x01)
#define PCI_MULTIMEDIA_TELEPHONY             (0x02)
#define PCI_MULTIMEDIA_AUDIO_DEVICE          (0x03)
#define PCI_MULTIMEDIA_MULTIMEDIA_OTHER      (0x80)

/**
 * @brief Literal constants pertaining to memory controllers
 */
#define PCI_CLASS_MEMORY_CONTROLLER          (0x05)
#define PCI_MEMORY_RAM                       (0x00)
#define PCI_MEMORY_FLASH                     (0x01)
#define PCI_MEMORY_MEMORY_OTHER              (0x80)

/**
 * @brief Literal contants pertaining to bridge devices
 */
#define PCI_CLASS_BRIDGE_DEVICE              (0x06)
#define PCI_BRIDGE_HOST                      (0x00)
#define PCI_BRIDGE_ISA                       (0x01)
#define PCI_BRIDGE_EISA                      (0x02)
#define PCI_BRIDGE_MICROCHANNEL              (0x03)
#define PCI_BRIDGE_PCI_BRIDGE                (0x04)
#define PCI_BRIDGE_PCMCIA                    (0x05)
#define PCI_BRIDGE_NUBUS                     (0x06)
#define PCI_BRIDGE_CARDBUS                   (0x07)
#define PCI_BRIDGE_RACEWAY                   (0x08)
#define PCI_BRIDGE_PCI_TO_PCI                (0x09)
#define PCI_BRIDGE_INFINIBAND                (0x0A)
#define PCI_BRIDGE_BRIDGE_OTHER              (0x80)

/**
 * @brief Literal constants pertaining to simple communication controllers
 */
#define PCI_CLASS_COMMUNICATION_CONTROLLER   (0x07)
#define PCI_COMM_SERIAL                      (0x00)
#define PCI_COMM_PARALLEL                    (0x01)
#define PCI_COMM_MULTIPORT_SERIAL            (0x02)
#define PCI_COMM_MODEM                       (0x03)
#define PCI_COMM_GPIB                        (0x04)
#define PCI_COMM_SMARTCARD                   (0x05)
#define PCI_COMM_COMMUNICATION_OTHER         (0x80)

/**
 * @brief Literal constants pertaining to base system peripherals
 */
#define PCI_CLASS_SYSTEM_PERIPHERAL          (0x08)
#define PCI_SYSTEM_PIC                       (0x00)
#define PCI_SYSTEM_DMA                       (0x01)
#define PCI_SYSTEM_TIMER                     (0x02)
#define PCI_SYSTEM_RTC                       (0x03)
#define PCI_SYSTEM_PCI_HOTPLUG               (0x04)
#define PCI_SYSTEM_SD_HOST_CONTROLLER        (0x05)
#define PCI_SYSTEM_SYSTEM_PERIPHERAL_OTHER   (0x80)

/**
 * @brief Literal constants pertainging to input device controllers
 */
#define PCI_CLASS_INPUT_DEVICE               (0x09)
#define PCI_INPUT_KEYBOARD                   (0x00)
#define PCI_INPUT_PEN                        (0x01)
#define PCI_INPUT_MOUSE                      (0x02)
#define PCI_INPUT_SCANNER                    (0x03)
#define PCI_INPUT_GAMEPORT                   (0x04)
#define PCI_INPUT_INPUT_DEVICE_OTHER         (0x80)

/**
 * @brief Literal constants pertaing to docking stations
 */
#define PCI_CLASS_DOCKING_STATION            (0x0A)
#define PCI_DS_GENERIC_DOCKING               (0x00)
#define PCI_DS_DOCKING_OTHER                 (0x80)

/**
 * @brief Literal constants pertaining to processors
 */
#define PCI_CLASS_PROCESSOR                  (0x0B)
#define PCI_PROCESSOR_386                    (0x00)
#define PCI_PROCESSOR_486                    (0x01)
#define PCI_PROCESSOR_PENTIUM                (0x02)
#define PCI_PROCESSOR_ALPHA                  (0x10)
#define PCI_PROCESSOR_POWERPC                (0x20)
#define PCI_PROCESSOR_MIPS                   (0x30)
#define PCI_PROCESSOR_CO_PROCESSOR           (0x40)

/**
 * @brief Literal contants pertaining to serial bus controllers
 */
#define PCI_CLASS_SERIAL_BUS                 (0x0C)
#define PCI_SERIAL_FIREWIRE                  (0x00)  // IEEE 1394
#define PCI_SERIAL_ACCESS_BUS                (0x01)
#define PCI_SERIAL_SSA                       (0x02)
#define PCI_SERIAL_USB                       (0x03)
#define PCI_SERIAL_FIBRE_CHANNEL             (0x04)
#define PCI_SERIAL_SMBUS                     (0x05)
#define PCI_SERIAL_INFINIBAND                (0x06)
#define PCI_SERIAL_IPMI                      (0x07)
#define PCI_SERIAL_SERCOS                    (0x08)
#define PCI_SERIAL_CANBUS                    (0x09)

/**
 * @brief Literal constants pertaining to wireless controllers
 */
#define PCI_CLASS_WIRELESS_CONTROLLER        (0x0D)
#define PCI_WIRELESS_IRDA                    (0x00)
#define PCI_WIRELESS_CONSUMER_IR             (0x01)
#define PCI_WIRELESS_RF                      (0x10)
#define PCI_WIRELESS_BLUETOOTH               (0x11)
#define PCI_WIRELESS_BROADBAND               (0x12)
#define PCI_WIRELESS_ETHERNET_CONTROLLER     (0x20)
#define PCI_WIRELESS_WIRELESS_OTHER          (0x80)

/**
 * @brief Literal constants pertaining to intelligent I/O controllers
 */
#define PCI_CLASS_INTELLIGENT_CONTROLLER     (0x0E)
#define PCI_IC_I2O                           (0x00)

/**
 * @brief Literal constants pertaining to satellite communication controllers
 */
#define PCI_CLASS_SATELLITE_CONTROLLER       (0x0F)
#define PCI_SATELLITE_TV                     (0x01)
#define PCI_SATELLITE_AUDIO                  (0x02)
#define PCI_SATELLITE_VOICE                  (0x03)
#define PCI_SATELLITE_DATA                   (0x04)

/**
 * @brief Literal constants pertaining to encryption/decrypion controllers
 */
#define PCI_CLASS_ENCRYPTION_CONTROLLER      (0x10)
#define PCI_CRYPT_NETWORK_AND_COMPUTING      (0x00)
#define PCI_CRYPT_ENTERTAINMENT              (0x10)
#define PCI_CRYPT_ENCRYPTION_OTHER           (0x80)

/**
 * @brief Literal constants pertaining to signal processing controllers
 */
#define PCI_CLASS_SIGNAL_PROCESSING          (0x11)
#define PCI_SP_DPIO                          (0x00)
#define PCI_SP_PERFORMANCE_COUNTERS          (0x01)
#define PCI_SP_COMMUNICATION_SYNCH           (0x10)
#define PCI_SP_SIGNAL_PROCESSING_OTHER       (0x80)

/**
 * @brief Literal constants for classes which are reserved
 */
#define PCI_CLASS_RESERVED                   (0x12)

/**
 * @brief Literal constants for processing accelerators 
 */
#define PCI_CLASS_ACCELERATORS               (0x14)
#define PCI_SUBCLASS_PROCESSING_ACCELERATOR  (0x00)

/**
 * @brief Literal constants for co-processors
 */
#define PCI_CLASS_CO_PROCESSOR               (0x40)
#define PCI_SUBCLASS_CO_PROCESSOR_GENERIC    (0x00)

/**
 * @brief Literal constants for non-standard class devices
 */
#define PCI_CLASS_OTHER_DEVICE               (0xFF)
#define PCI_SUBCLASS_OTHER                   (0x00)

/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
PCI_BAR find_bar(uint32_t bus, uint32_t slot, uint32_t func, uint8_t bar);
void pci_scan_bus(uint8_t bus);
void pci_list();
void pci_init();