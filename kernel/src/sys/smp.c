/**
 * @file smp.c
 * @author Zack Bostock
 * @brief Functionality for Symmetric Multiprocessing (SMP)
 * @note AP = Application Processor (opposite of Bootstrap Processor)
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <kconfig.h>
#include <const.h>

#include <common/kprint.h>
#include <common/lock.h>

#include <sys/smp.h>
#include <sys/cpu.h>
#include <sys/mmu.h>
#include <sys/gdt/gdt.h>
#include <sys/acpi/apic.h>
#include <sys/tick/clkhandler.h>

#include <proc/ctxsw.h>
#include <proc/syscall.h>

extern uint8_t smp_trampoline_blob_start;
extern uint8_t smp_trampoline_blob_end;

static volatile int *ap_boot_counter = (volatile int *)
                                       PHYS_TO_VIRT(SMP_AP_BOOT_COUNTER_ADDR);
static SMP_INFO *smp_info = NULL;
static volatile int smp_initialized = 0;
static LOCK smp_lock = {0};

/**
 * @brief Helper to get the smp initalization information
 *
 * @return SMP_INFO* smp_info structure
 */
SMP_INFO *smp_get_info() {
    if (!smp_initialized) {
        return NULL;
    }
    return smp_info;
}

/**
 * @brief Helper to get the current CPU's info structure
 *
 * @param force Make the MSR be read regardless if SMP has been initialized
 * @return CPU* Structure for the current CPU
 */
CPU *smp_get_curr_cpu(int force) {
    if (smp_initialized || force) {
        CPU *cpu = (CPU *) (read_msr(MSR_KERN_GS_BASE_ADDR));
        if (!cpu) {
            cpu = (CPU *) (read_msr(MSR_GS_BASE_ADDR));
        }
        return cpu;
    }
    return NULL;
}

/**
 * @brief Helper function to set the error number on the CPU
 * @note This function will be called frequently in system calls
 *
 * @param errno Error number to set
 * @return STATUS SYS_OK if success, SYS_ERR if failure
 */
STATUS cpu_set_errno(int64_t errno) {
    LOCK_LOCK(&smp_lock);
    if (smp_initialized) {
        CPU *cpu = (CPU *) (read_msr(MSR_KERN_GS_BASE_ADDR));
        if (!cpu) {
            cpu = (CPU *) (read_msr(MSR_GS_BASE_ADDR));
        }
        if (cpu) {
            cpu->errno = errno;
            UNLOCK_LOCK(&smp_lock);
            return SYS_OK;
        }
    }
    UNLOCK_LOCK(&smp_lock);
    return SYS_ERR;
}

/**
 * @brief Helper function to call the TSS init function
 *
 * @param cpu_info CPU to call the TSS init function on
 */
void init_tss(CPU *cpu_info) {
    gdt_init_tss(cpu_info);
}

/**
 * @brief Main entry point for the AP's
 *
 * @param cpu_info Information of the AP which is entering
 */
__attribute__((noreturn)) void smp_ap_entrypoint(CPU *cpu_info) {

    klogs("INIT SMP: starting...\n");

    /* Initialize CPU features */
    cpu_init(cpu_info->cpu_id);
    gdt_init(cpu_info);

    /* Put the CPU information in GS */
    write_msr(MSR_GS_BASE_ADDR, (uint64_t) cpu_info);
    write_msr(MSR_KERN_GS_BASE_ADDR, (uint64_t) cpu_info);

    /* Initialize GDT and make a TSS */
    for (size_t dl = 0; dl < 100; dl++) {
        __asm__ volatile("nop");
    }
    init_tss(cpu_info);

    /* Enable the APIC */
    apic_enable();

    /* Enable System Calls for this CPU */
    system_calls_init();

    /* Initialize and wait for scheduler */
    klogd("Initialize schedulder for cpu %d\n", cpu_info->cpu_id);
    ctxsw_init("init", cpu_info->cpu_id);

    while (!smp_initialized) {
        system_timer_sleep(10);
    }

    enable_interrupts();

    klogs("INIT SMP: Initialized core %d (%x)\n", cpu_info->cpu_id, cpu_info);

    while(1) {
        __asm__ volatile("hlt");
    }
}

/**
 * @brief Helper to get the SMP trampoline set up
 */
static void prepare_trampoline() {
    /* Copy the trampoline blob to 0x1000 physical memory address */
    uint64_t tblobsize = (uint64_t)(&smp_trampoline_blob_end) -
                         (uint64_t)(&smp_trampoline_blob_start);

    memcpy((void *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_BLOCK_ADDR)),
           &smp_trampoline_blob_start, tblobsize);

    /* Pass arguments to trampoline */
    uint64_t *addr = (uint64_t *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_ARG_CR3));
    *addr = read_cr(cr3);
    __asm__ volatile("sidt %0"
                     : "=m"(*(uint64_t *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_ARG_IDTPTR)))
                     :
                     :);
    /* Setting the entry point for the new CPU to be the ap_entrypoint function */
    *((uint64_t *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_ARG_ENTRYPOINT))) =
        (uint64_t)(&smp_ap_entrypoint);
    klogi("INIT SMP: Trampoline %x to %x\n",
          (uint64_t)(&smp_trampoline_blob_start),
          (uint64_t)(&smp_trampoline_blob_end));
}

/**
 * @brief Main initialization function for
 */
void smp_init() {
    klogs("INIT SMP: starting...\n");
    smp_info = (SMP_INFO *)(kmalloc(sizeof(SMP_INFO)));
    memset(smp_info, 0, sizeof(SMP_INFO));

    /* Indentity map first MB for the trampoline */
    vm_map(NULL, 0, 0, NUM_PAGES(0x100000), VM_DEFAULT);

    /* Set up the different components and addresses for the trampoline */
    prepare_trampoline();

    /* Get Local APIC info from MADT */
    uint64_t cpu_num = madt_get_num_local_apics();
    MADT_RECORD_LAPIC **lapics = madt_get_local_apics();

    #if !BSP_ONLY
    klogd("INIT SMP: Initializing core %d...\n", cpu_num);
    #endif

    /* We must have a BSP core whose id is zero */
    memset(&(smp_info->cpus[0]), 0, sizeof(CPU));

    smp_info->cpus[0].cpu_id = 0;
    for (size_t i = 0; i < cpu_num; i++) {
        if (apic_read_reg(APIC_LAPIC_ID_REG) == lapics[i]->apic_id) {
            smp_info->cpus[0].lapic_id = lapics[i]->apic_id;
            smp_info->cpus[0].proc_id = lapics[i]->process_id;
            klogi("INIT SMP: Core 0 with process id %d is the BSP\n",
                  lapics[i]->process_id);
            break;
        }
    }

    smp_info->cpus[0].is_bootstrap_processor = TRUE;
    write_msr(MSR_GS_BASE_ADDR, (uint64_t)&(smp_info->cpus[0]));
    write_msr(MSR_KERN_GS_BASE_ADDR, (uint64_t)&(smp_info->cpus[0]));

    /* Initialize task state segment */
    init_tss(&(smp_info->cpus[0]));

    smp_info->num_cpus = 1;

    /* Loop through the lapic's present and initialize them one by one */
    #if BSP_ONLY
    /* Won't be using this parameter if only using bootstrap processor */
    (void) ap_boot_counter;
    #else
    for (size_t i = 0; i < cpu_num; i++) {
        size_t core = 0;
        if (apic_read_reg(APIC_LAPIC_ID_REG) != lapics[i]->apic_id) {
            core = smp_info->num_cpus;
        } else {
            /* This is the bootstrap processor */
            continue;
        }

        memset(&(smp_info->cpus[core]), 0, sizeof(CPU));
        int counter_prev = *ap_boot_counter;

        /* If CPU is not able to come online, just skip it */
        if (!(lapics[i]->flags & MADT_LAPIC_FLAG_ONLINE_CAPABLE) &&
            !(lapics[i]->flags & MADT_LAPIC_FLAG_ENABLED)) {
            klogi("INIT SMP: core %d with process ID %d is not enabled or online "
                  "capable\n", core, lapics[i]->process_id);
            continue;
        }

        smp_info->cpus[core].cpu_id = core;
        smp_info->cpus[core].lapic_id = lapics[i]->apic_id;
        smp_info->cpus[core].proc_id = lapics[i]->process_id;

        klogi("INIT SMP: Initializing core %d with APIC ID %x...\n",
              core, lapics[i]->apic_id);

        /* Allocate and pass the stack to the new processor */
        void *stack = kmalloc(STACK_SIZE);
        *((uint64_t *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_ARG_RSP))) = (uint64_t)stack +
                                                                STACK_SIZE;

        /* Pass the CPU's information */
        *((uint64_t *)(PHYS_TO_VIRT(SMP_TRAMPOLINE_ARG_CPUINFO))) =
            (uint64_t)&(smp_info->cpus[core]);

        /* Send the init Inter-processor Interrupt (IPI) */
        apic_send_ipi(lapics[i]->apic_id, 0, APIC_IPI_MTYPE_INIT);

        /* Make sure the interrupt was received */
        sched_sleep(10);

        uint8_t success = FALSE;

        /* Send startup IPI twice */
        for (size_t k = 0; k < 2; k++) {
            apic_send_ipi(lapics[i]->apic_id,
                          SMP_TRAMPOLINE_BLOCK_ADDR / PAGE_SIZE,
                          APIC_IPI_MTYPE_STARTUP);
            /* Check if the CPU has started */
            for (size_t j = 0; j < 20; j++) {
                int curr_counter = *ap_boot_counter;
                if (curr_counter != counter_prev) {
                    success = TRUE;
                    break;
                }
                sched_sleep(1);
            }

            if (success) {
                break;
            }
        }

        if (!success) {
            klogi("INIT SMP: core %d initialization failed\n", core);
            kfree(stack);
        } else {
            klogi("SMP: core %d initialization succeeded\n", core);
            smp_info->cpus[smp_info->num_cpus].is_bootstrap_processor = FALSE;
        }
        smp_info->num_cpus++;
    }

    while (TRUE) {
        if (sched_get_cpu_num() == smp_info->num_cpus - 1) {
            break;
        }
        system_timer_sleep(1);
    }
    #endif

    klogi("INIT SMP: %d processors brought up\n", smp_info->num_cpus);

    /* Identidy mapping is no longer needed */
    vm_unmap(NULL, 0, NUM_PAGES(0x100000));

    smp_initialized = TRUE;

    enable_interrupts();
    klogs("INIT SMP: finished...\n");
}