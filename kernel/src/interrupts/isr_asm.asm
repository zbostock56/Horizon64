bits 64
section .text
extern isr_handler

%macro pushall 0
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro ISR_NOERRORCODE 1
global ISR%1
ISR%1:
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]

    pushall

    mov rdi, %1
    mov rsi, rsp
    mov qword [rsp + 40], 0

    call isr_handler
    jmp isr_exit
%endmacro

%macro ISR_ERRORCODE 1
global ISR%1
ISR%1:
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]
    push qword [5 * 8 + rsp]

    pushall

    mov rdi, %1
    mov rsi, rsp
    mov rdx, qword [20 * 8 + rsp]

    call isr_handler
    jmp isr_exit
%endmacro

%include "include/isrs_gen.inc"

isr_exit:
    popall
    add rsp, 40
    iretq