%include "src/sys/asm_macros.asm"

global syscall_entry
global syscall_handler

extern syscall_funcs

syscall_entry:
    ; When entering the syscall handler, the cpu puts the previous value of RIP
    ; in RCX and the previous value of RFLAGS in R11. The processor also uses
    ; them to reset the value of RIP and RFLAGS when the syscall returns
    push r11
    push rcx

    ; System V ABI AMD64 Arch convention (section AMD64 Linux Kernel Conventions)
    mov rax, rdi            ; syscall number
    mov rdi, rsi            ; 1st argument
    mov rsi, rdx            ; 2nd argument
    mov rdx, rcx            ; 3rd argument
    mov r10, r8             ; 4th argument
    mov r8, r9              ; 5th argument
    mov r9, [rsp + 0x18]    ; 6th argument

    syscall                 ; RAX now contains the return value of the syscall

    ; Restore RIP and RFLAGS
    pop rcx
    pop r11

    ret

syscall_handler:
    push r15                ; store r15 in user stack
    mov r15, rsp            ; save process's stack in r15

    push r11                ; Saved RFLAGS
    push rcx                ; Saved RIP

    pushall                 ; push all registers using macro

    mov rcx, r10

    call [rax * 8 + syscall_funcs]

    popall_syscall          ; Pop all registers except for RAX

    mov rdx, qword [gs:0x0] ; Return errno in rdx
    mov rsp, r15            ; Restore user stack
    pop r15                 ; Pop r15 from user stack

    o64 sysret              ; Return to user mode
                            ; Restores RIP from RCX
                            ; Restores RFLAGS from R11
