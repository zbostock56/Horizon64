/**
 * @file kernel.c
 * @author Zack Bostock
 * @brief Main entry point for kernel
 * @verbatim
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <kernel.h>

#include <kconfig.h>

#include <proc/ctxsw.h>

#include <fs/ttyfs.h>
#include <fs/pipefs.h>

#include <dev/storage/ata.h>
#include <dev/terminal.h>

#include <sys/smp.h>

/**
 * @brief Starts up the shell
 */
__attribute__((noreturn)) void kshell(PROC_ID id) {
    (void) id;
    init_ttyfs();
    init_pipefs();
    init_ata();

    terminal_enable_character_printing();
    kprintf("Executing /bin/init...\n");

    sched_execve("/bin/init", NULL, NULL, "/root");

    /* This now becomes the idle process */
    PROCESS *p = sched_get_curr_proc();
    if (p) {
        process_idle_proc(p->id);
    } else {
        klog("kshell does not have a process id!\n");
        halt();
    }

    /* Should not reach here */
    while (1) {}
}

/**
 * @brief Handles the cursor
 */
__attribute__((noreturn)) void kcursor(PROC_ID id) {
    (void) id;
    klogd("%s-%d: started kcursor\n", __func__, id);
    while (TRUE) {
        klogd("%s-%d: going to sleep...\n", __func__, id);
        sched_sleep(500);
        klogd("%s-%d: woke up...\n", __func__, id);
        if (cursor_visible == TERM_CURSOR_INVISIBLE) {
            terminal_set_cursor(219);
            cursor_visible = TERM_CURSOR_VISIBLE;
        } else if (cursor_visible == TERM_CURSOR_VISIBLE) {
            terminal_set_cursor(' ');
            cursor_visible = TERM_CURSOR_INVISIBLE;
        } else {
            terminal_set_cursor(' ');
        }
        terminal_refresh(TERM_MODE_TERM);
    }
}

/**
 * @brief Main entry point from bootloader to kernel.
 */
void _start() {

    /* Sets vital system settings */
    system_init();

    /* Add the cursor process */
    klogi("Kernel: Adding the kcursor process...\n");
    sched_add(sched_new("kcursor", kcursor, FALSE));

    /* Add the shell process */
    klogi("Kernel: Adding the kshell process...\n");
    sched_add(sched_new("kshell", kshell, FALSE));

    CPU *cpu = smp_get_curr_cpu(NO_FORCE_GET_CPU);
    if (cpu) {
        /*
            NOTE:
            This should only run on the bootstrap processor. The initialization
            for the other application processors should occur in their own SMP
            initialization phase.
        */
        ctxsw_init("init", cpu->cpu_id);
        enable_interrupts();
    } else {
        kloge("Cannot get CPU info for null process!\n");
        halt();
    }

    /* Wait for the first interrupt to be received by the APIC timer */
    while (1) {
        asm("hlt;");
    }
}
