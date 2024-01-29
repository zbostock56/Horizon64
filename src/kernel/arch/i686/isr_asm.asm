bits 32

extern i686_isr_handler

%macro ISR_NOERRORCODE 1
global i686_ISR%1
i686_ISR%1:
    push 0              ; Push dummy error code
    push %1             ; Push interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERRORCODE 1
global i686_ISR%1:
i686_ISR%1:
    push %1             ; Push interrupt number
    jmp isr_common
%endmacro

%include "arch/i686/gen_isrs.inc"

isr_common:
    pusha

    xor eax, eax        ; push ds
    mov ax, ds
    push eax

    mov ax, 0x10        ; use kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call i686_isr_handler
    add esp, 4

    pop eax
    mov ds, ax          ; Restore previous segment
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iret