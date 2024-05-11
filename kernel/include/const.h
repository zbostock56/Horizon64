#pragma once

/* Default for scheduling quantum time */
#define QUANTUM (5)
#define PIT_1MS (1000)

/* Programmable interrupt controller (PIC) */
#define PIC_REMAP_OFFSET        (0x20)
#define NUM_HARDWARE_INTERRUPTS (0x10)

/* -------------------------------- PSF Fonts ------------------------------- */
/* structs/framebuffer_str.h */
#define PSF1         (0)
#define PSF2         (1)
/* psf.h */
#define PSF1_SUCCESS (0)
#define NOT_PSF1     (1)
#define PSF2_SUCCESS (0)
#define NOT_PSF2     (1)

#define PSF1_FONT_WIDTH (8)

/* SMP */
#define NUM_CPUS (1)

/* Keyboard */
#define ARROW_UP        (0x48)
#define BACKSPACE       (0x0E)
#define CAPS_LOCK       (0x3A)
#define ENTER           (0x1C)
#define LEFT_CONTROL    (0x1D)    /* Same as the right control key */
#define LEFT_SHIFT      (0x2A)
#define RIGHT_SHIFT     (0x36)
#define TAB             (0x0F)

#define SHIFT_ENABLED   (0x01)
#define SHIFT_DISABLED  (0x00)