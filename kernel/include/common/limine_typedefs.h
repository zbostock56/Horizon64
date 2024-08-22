/**
 * @file limine_typedefs.h
 * @author Zack Bostock
 * @brief A helper header file for shortening the long types from Limine
 *
 * @copyright Copyright (c) 2024
 *
 */


#pragma once
#include <limine.h>

typedef struct limine_file LIMINE_FILE;
typedef struct limine_module_response LIMINE_MODULE_RESP;
typedef struct limine_module_request LIMINE_MODULE_REQ;
typedef struct limine_framebuffer_request LIMINE_FRAMEBUFF_REQ;
typedef struct limine_memmap_request LIMINE_MEM_REQ;
typedef struct limine_memmap_response LIMINE_MEM_RES;
typedef struct limine_kernel_address_response LIMINE_K_ADDR_RES;
typedef struct limine_kernel_address_request LIMINE_K_ADDR_REQ;
typedef struct limine_rsdp_request LIMINE_RSDP_REQ;
typedef struct limine_rsdp_response LIMINE_RSDP_RES;
typedef struct limine_bootloader_info_request LIMINE_BL_INFO_REQ;
typedef struct limine_bootloader_info_response LIMINE_BL_INFO_RES;
