/**
 * @file panic.c
 * @author Zack Bostock
 * @brief Dumps information about the kernel and its execution when its state
 * is unrecoverable
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>

#include <structs/symbols_str.h>

#include <sys/smp.h>
#include <sys/ksymbols.h>

#include <common/kprint.h>
#include <common/lock.h>

/**
 * @brief Helper to find a symbol in the kernel map based on the program counter
 *
 * @param addr Address of symbol to find
 * @return int Symbol number, or -1 if not found
 */
static inline int symbols_get_index(uint64_t addr) {
    for (int i = 0; _kernel_symbols[i].addr < UINT64_MAX; i++) {
        if (_kernel_symbols[i].addr < addr &&
            _kernel_symbols[i + 1].addr >= addr) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Dumps the backtrace of the current RBP
 */
void backtrace(uint64_t rip) {
    if (rip) {
        int idx = symbols_get_index(rip);
        if (idx < 0) {
            klogn("Problematic Instruction:\n\t(Unknown Function): %x\n", rip);
        } else {
            klogn("Problematic Instruction:\n\tBase: %x\tCrash Point: %x (%s + %04x)\n",
                                                _kernel_symbols[idx].addr,
                                                rip, _kernel_symbols[idx].name,
                                                rip - _kernel_symbols[idx].addr);
        }
    }

    uint64_t *rbp = 0;
    __asm__ volatile("mov %%rbp, %0" : "=g" (rbp) :: "memory");

    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (cpu) {
        klogn("\nStacktrace (CPU %d):\n", cpu->cpu_id);
    } else {
        klogn("\nStacktrace:\n");
    }
    for (size_t i = 0;; i++) {
        uint64_t func_addr = *(rbp + 1);
        rbp = (uint64_t *) *rbp;
        if (func_addr == (uint64_t) NULL || rbp == NULL) {
            break;
        }

        int idx = symbols_get_index(func_addr);
        if (idx < 0) {
            klogn("[%02d]\t%x (Unknown Function)\n", i, func_addr);
        } else {
            klogn("[%02d]\tBase: %x\tCrash Point: %x (%s + %04x)\n", i, _kernel_symbols[idx].addr,
                                               func_addr, _kernel_symbols[idx].name,
                                               func_addr - _kernel_symbols[idx].addr);
        }
    }

    klogn("\nEnd of stacktrace.\n");
}