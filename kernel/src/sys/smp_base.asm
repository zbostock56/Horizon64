LOAD_ADDRESS        equ 0x1000
AP_BOOT_COUNTER     equ 0xFF0
ARG_CPUINFO         equ 0xFE0
ARG_CR3_VAL         equ 0xFD0
ARG_ENTRYPOINT      equ 0xFC0
ARG_RSP             equ 0xFB0
ARG_IDTPTR          equ 0xFA0

bits 16                             ; Start in Real Mode

rmode_entry:
    cli
    jmp load_gdt32

gdt32_start:
    dq 0x0000000000000000           ; Null Segment
    dq 0x00DF9A000000FFFF           ; Code Segment
    dq 0x00DF92000000FFFF           ; Data Segment

gdt32_ptr:
    dw 32
    dd LOAD_ADDRESS + gdt32_start

gdt64_start:
    dq 0x0000000000000000           ; Null Segment
    dq 0x00AF9A000000FFFF           ; Code Segment
    dq 0x008F92000000FFFF           ; Data Segment

gdt64_ptr:
    dw 23
    dq LOAD_ADDRESS + gdt64_start

load_gdt32:                         ; Load the GDT for Protected Mode
    lgdt [cs:gdt32_ptr]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp dword 0x08:LOAD_ADDRESS + pmode_entry

bits 32                             ; Now outputting protected mode code

pmode_entry:
    ; Update segment registers
    mov eax, 0x10
    mov es, eax
    mov ss, eax
    mov ds, eax
    mov fs, eax
    mov fs, eax
    mov gs, eax

    ; Set PAE enable bit in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Update CR3
    mov eax, [ARG_CR3_VAL]
    mov cr3, eax

    ; Enable long mode by setting MLE flag (bit 8) in MSR
    mov eax, 0xC0000080
    rdmsr
    or eax, 1 << 8                  ; Set the flag
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [LOAD_ADDRESS + gdt64_ptr]
    jmp 0x08:LOAD_ADDRESS + lmode_entry

bits 64                                 ; Outputting Long Mode code now

lmode_entry:
    lidt [ARG_IDTPTR]                   ; Load IDT
    mov rsp, [ARG_RSP]                  ; Initialize Stack
    mov rsi, [ARG_CPUINFO]              ; Pass CPU information into kernel code
    lock add word [AP_BOOT_COUNTER], 1  ; Increment counter to indicate successful boot
    call [ARG_ENTRYPOINT]               ; Jump to kernel C code