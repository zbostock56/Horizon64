/**
 * @file getclock.c
 * @author Zack Bostock
 * @brief Functionality pertaining to getclock system call
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <proc/syscalls/getclock.h>
#include <proc/syscall.h>
#include <proc/ctxsw.h>

#include <sys/smp.h>
#include <sys/acpi/hpet.h>

#include <sys/cmos.h>

/**
 * @brief Gets clock time
 *
 * @param unused Unused (here for compatibilty)
 * @param which Which time to get
 * @param out Buffer to copy result into
 * @return int64_t 0 if success, -1 if failure
 */
int64_t sys_getclock(void *unused, int64_t which, VFS_TIMESPEC *out) {
    (void)unused;
    cpu_set_errno(0);


    uint64_t now_ns = hpet_get_nanos();
    uint64_t now_sec = now_ns / 1000000000;

    uint64_t boot_time = cmos_get_boot_time_seconds();

    switch (which) {
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_COARSE:
            *out = (VFS_TIMESPEC) {
                .TV_SEC = now_sec + boot_time,
                .TV_NSEC = now_ns + boot_time * 1000000000
            };
            return 0;
        case CLOCK_BOOTTIME:
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_MONOTONIC_COARSE:
            *out = (VFS_TIMESPEC) {
                .TV_SEC = now_sec,
                .TV_NSEC = now_ns
            };
            return 0;
        case CLOCK_PROC_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID:
            *out = (VFS_TIMESPEC) {
                .TV_SEC = 0,
                .TV_NSEC = 0
            };
            return 0;
    }

    cpu_set_errno(EINVAL);
    return -1;
}
