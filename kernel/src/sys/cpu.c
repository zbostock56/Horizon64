/**
 * @file cpu.c
 * @author Zack Bostock
 * @brief Initializes CPU specific features
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <sys/cpu.h>
#include <sys/cpu_features.h>
#include <common/memory.h>

static char cpu_manufacturer[13] = {0};
/*
    Some predefined vendors that GCC recognizes where the value of EBX is
    already known.
*/

static VENDOR vendor_intel = {
    .ebx = signature_INTEL_ebx,
    .vendor_name = CPUID_VENDOR_INTEL
};

static VENDOR vendor_amd = {
    .ebx = signature_AMD_ebx,
    .vendor_name = CPUID_VENDOR_AMD
};



/**
 * @brief Helper function to check if a cpu feature is supported
 *
 * @param feature Feature to check if compatible
 * @return STATUS SYS_ERR if no, SYS_OK if yes
 */
STATUS check_cpu_support(CPUID_FEATURE feature) {
    uint32_t registers[4];
    cpuid(feature.feature, &registers[CPUID_EAX], &registers[CPUID_EBX],
          &registers[CPUID_ECX], &registers[CPUID_EDX]);

    if (registers[feature.registers] & feature.mask) {
        return SYS_OK;
    } else {
        return SYS_ERR;
    }
}

/**
 * @brief Get the cpu vendor
 *
 * @param copy_to Buffer to copy manufacturer string into
 */
void get_cpu_vendor(char *copy_to) {
     if (cpu_manufacturer[0] != '\0') {
         memcpy(copy_to, cpu_manufacturer, 13);
     }
     uint32_t registers[4];
     if (cpuid(0, &registers[CPUID_EAX], &registers[CPUID_EBX],
              &registers[CPUID_ECX], &registers[CPUID_EDX]) == SYS_OK) {
         if (registers[CPUID_EBX] == vendor_intel.ebx) {
             memcpy(copy_to, vendor_intel.vendor_name, 12);
         } else if (registers[CPUID_EBX] == vendor_amd.ebx) {
             memcpy(copy_to, vendor_amd.vendor_name, 12);
         } else {
             ((unsigned int *) copy_to)[0] = registers[CPUID_EBX];
             ((unsigned int *) copy_to)[2] = registers[CPUID_ECX];
             ((unsigned int *) copy_to)[1] = registers[CPUID_EDX];
         }
         /* Put a null terminator at end of string */
         copy_to[12] = '\0';
     } else {
         memcpy(copy_to, "Unknown\0", 13);
     }
}

/**
 * @brief Get the cpu model name
 *
 * @param copy_to Buffer to copy model name into
 */
void get_cpu_model_name(char *copy_to) {
    uint32_t registers[4];
    __cpuid(0x80000002, registers[CPUID_EAX], registers[CPUID_EBX],
            registers[CPUID_ECX], registers[CPUID_EDX]);
    ((uint32_t *) copy_to)[0] = registers[CPUID_EAX];
    ((uint32_t *) copy_to)[1] = registers[CPUID_EBX];
    ((uint32_t *) copy_to)[2] = registers[CPUID_ECX];
    ((uint32_t *) copy_to)[3] = registers[CPUID_EDX];
    __cpuid(0x80000004, registers[CPUID_EAX], registers[CPUID_EBX],
            registers[CPUID_ECX], registers[CPUID_EDX]);
    ((uint32_t *) copy_to)[4] = registers[CPUID_EAX];
    ((uint32_t *) copy_to)[5] = registers[CPUID_EBX];
    ((uint32_t *) copy_to)[6] = registers[CPUID_ECX];
    ((uint32_t *) copy_to)[7] = registers[CPUID_EDX];
    __cpuid(0x80000004, registers[CPUID_EAX], registers[CPUID_EBX],
            registers[CPUID_ECX], registers[CPUID_EDX]);
    ((uint32_t *) copy_to)[8] = registers[CPUID_EAX];
    ((uint32_t *) copy_to)[9] = registers[CPUID_EBX];
    ((uint32_t *) copy_to)[10] = registers[CPUID_ECX];
    ((uint32_t *) copy_to)[11] = registers[CPUID_EDX];
    copy_to[48] = '\0';
}

/**
 * @brief Get the CPU family, stepping, and model number
 *
 * @param f Family of the CPU
 * @param stepping Stepping of the CPU
 * @param mn Model number of the CPU
 */
void get_cpu_family(int *f, int *stepping, int *mn) {
    uint32_t registers[4];
    __cpuid(0x1, registers[CPUID_EAX], registers[CPUID_EBX],
            registers[CPUID_ECX], registers[CPUID_EDX]);
    uint32_t family = (registers[CPUID_EAX] >> 8) & 0xF;
    uint32_t model_num = (registers[CPUID_EAX] >> 4) & 0xF;
    *stepping = registers[CPUID_EAX] & 0xF;
    uint32_t ext_model = (registers[CPUID_EAX] >> 16) & 0xF;
    uint32_t ext_family = (registers[CPUID_EAX] >> 20) & 0xFF;

    if (family == 0xF) {
        *f += ext_family;
    }
    if (family == 0x6 || family == 0xF) {
        *mn = (model_num + (ext_model << 4));
    }
}

/**
 * @brief Get the CPU cache info
 *
 * @param cache_line_size How many bytes is the cache line
 * @param l2_cache_size How many Kilobytes of L2 cache
 * @param l3_cache_size How many Kilobytes of L3 cache
 */
void get_cpu_cache_info(int *cache_line_size, int *l2_cache_size,
                        int *l3_cache_size) {
    uint32_t registers[4];
    __cpuid(0x80000006, registers[CPUID_EAX], registers[CPUID_EBX],
            registers[CPUID_ECX], registers[CPUID_EDX]);
    *cache_line_size = registers[CPUID_ECX] & 0xFF;
    *l2_cache_size = (registers[CPUID_ECX] >> 16) & 0xFFFF;
    *l3_cache_size = (registers[CPUID_EDX] >> 18) & 0x3FFF;
}

/**
 * @brief Helper to call all the CPU information getters and print in readable
 *        fashion.
 */
void print_cpu_info() {
    char vendor[13] = {0};
    int family = 0;
    int model_number = 0;
    int stepping = 0;
    char brand[49] = {0};
    int cache_line_size = 0;
    int l2_cache_size = 0;
    int l3_cache_size = 0;

    get_cpu_vendor(vendor);
    get_cpu_family(&family, &stepping, &model_number);
    get_cpu_model_name(brand);
    get_cpu_cache_info(&cache_line_size, &l2_cache_size, &l3_cache_size);

    klogi("Vendor ID:   %s\n", vendor);
    klogi("CPU family:  %d\n", family);
    klogi("Model:       %d\n", model_number);
    klogi("Model name:  %s\n", brand);
    klogi("Stepping:    %d\n", stepping);
    klogi("Cache Lines: %d Bytes\n", cache_line_size);
    klogi("L2 cache:    %d KB\n", l2_cache_size);
    klogi("L3 cache:    %d KB\n", l3_cache_size);
}

/**
 * @brief Initialization function for setting up cpu specific features
 *
 * @param cpu_number Denotes which cpu to start
 */
void cpu_init(size_t cpu_number) {

    klogi("INIT CPU %d: starting...\n", cpu_number);

    /* Check for PAT support and enable */
    /*
        NOTE: Functionality -
            Uncacheable pages:
                The operating system can mark pages as
                uncacheable, preventing the CPU from caching them.
            Write-combining pages:
                The operating system can mark pages as
                write-combining, allowing the CPU to combine
                multiple writes to the same page into a single write operation.
            Write-through pages:
                The operating system can mark pages as write-through, allowing
                the CPU to write data directly to memory without caching it.
    */
    if (check_cpu_support(cpuid_feature_pat)) {
        klogi("CPU INIT: CPU %d enabling page attribute table\n", cpu_number);
        uint64_t pat_value = read_msr(MSR_PAT);
        pat_value &= (uint64_t) (0xFFFFFFFF);
        pat_value |= ((uint64_t) (0x1)) << 32;
        write_msr(MSR_PAT, pat_value);
    }

    /* OSFXSR: Enables 128-bit SSE support.   */
    /* OSXMMEXCPT: Enables the #XF exception. */
    /*
        NOTE: The OSXMMEXCPT bit is used in conjunction with the OSFXSR bit
              to determine how the processor handles SIMD floating-point
              exceptions. When both bits are set, the processor will raise an
              exception to the operating system when a SIMD floating-point
              exception occurs.
    */
    WRITE_TO_CR4_BIT(CR4_FXSAVE_FXRSTOR_INSTRUCTIONS);
    WRITE_TO_CR4_BIT(CR4_UNMASKED_SIMD_FLOATING_POINT_EXCEPTIONS);

    /* Print out the CPU manufacturer */
    klogi("Printing out CPU %d's info\n", cpu_number);
    print_cpu_info();
    klogi("INIT CPU %d: finished...\n", cpu_number);
}
