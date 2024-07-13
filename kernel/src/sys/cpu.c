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

/**
 * @brief Uses the the cpuid function from gcc to determine the availability
 *        of CPU specific features.
 *
 * @param feature Specific feature to check
 * @note feature will be OR'd with 0x80000000 upon the call to __get_cpuid
 * @ref https://github.com/gcc-mirror/gcc/blob/master/gcc/config/i386/cpuid.h
 *
 * @param eax EAX register to pass into cpuid
 * @param ebx EBX register to pass into cpuid
 * @param ecx ECX register to pass into cpuid
 * @param edx EDX register to pass into cpuid
 */
static inline STATUS cpuid(uint32_t feature, uint32_t *eax, uint32_t *ebx,
                           uint32_t *ecx, uint32_t *edx) {
    unsigned int ret = __get_cpuid(feature, eax, ebx, ecx, edx);
    if (ret != 1) {
        klogi("CPUID failed with feature input %x!!\n", feature);
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
 * @brief Reads the model specific register specified.
 *
 * @param msr Certain model specific register
 * @return uint64_t Current value in msr
 */
static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;

    __asm__ volatile("mov %[msr], %%ecx;"
                     "rdmsr;"
                     "mov %%eax, %[low];"
                     "mov %%edx, %[high];"
                     : [low] "=g"(low), [high] "=g"(high)
                     : [msr] "g"(msr)
                     : "eax", "ecx", "edx");

    return (((uint64_t) high << 32) | low);
}

/**
 * @brief Writes 64 bits to a model specific register.
 *
 * @param msr MSR to write to
 * @param val Value to write to MSR.
 */
static inline void write_msr(uint32_t msr, uint64_t val) {
    uint32_t low = val & UINT32_MAX;
    uint32_t high = (val >> 32) & UINT32_MAX;

    __asm__ volatile("mov %[msr], %%ecx;"
                     "mov %[low], %%eax;"
                     "mov %[high], %%edx;"
                     "wrmsr;"
                     :
                     : [msr] "g"(msr), [low] "g"(low), [high] "g"(high)
                     : "eax", "ecx", "edx");
}

/**
 * @brief Helper function to check if a cpu feature is supported
 *
 * @param feature Feature to check if compatible
 * @return STATUS SYS_ERROR if no, SYS_OK if yes
 */
STATUS check_cpu_support(CPUID_FEATURE feature) {
    uint32_t registers[4];
    cpuid(feature.feature, &registers[CPUID_EAX], &registers[CPUID_EBX],
          &registers[CPUID_ECX], &registers[CPUID_EDX]);

    if (registers[feature.registers] & feature.mask) {
        return SYS_OK;
    } else {
        return SYS_ERROR;
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
    int family;
    int model_number;
    int stepping;
    char brand[49] = {0};
    int cache_line_size;
    int l2_cache_size;
    int l3_cache_size;

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
    klogi("L2 cache:    %dK\n", l2_cache_size);
    klogi("L3 cache:    %dK\n", l3_cache_size);
}

/**
 * @brief Initialization function for setting up cpu specific features
 *
 * @param cpu_number Denotes which cpu to start
 */
void cpu_init(size_t cpu_number) {

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
        klogi("CPU INIT: cpu %d enabling page attribute table\n", cpu_number);
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
    klogi("CPU INIT: Printing out CPU %d's info\n", cpu_number);
    print_cpu_info();
    klogi("Initialization for CPU %d finished...\n", cpu_number);
}