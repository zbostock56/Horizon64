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

; @ref Intel Systems Programmer Guide Section 5.8.8
;
; SYSCALL:
; For SYSCALL, the processor saves RFLAGS into R11 and the RIP of the next
; instruction into RCX; it then gets the privilege-level 0 target code segment,
; instruction pointer, stage segment and flags as follows:
;
; Target Code Segment - Reads a non-NULL selector from IA32_STAR[47:32] 
; Target Instruction Pointer - Reads a 64-bit address from IA32_LSTAR
; Stack Segment - Computed by adding 8 to the value in IA32_STAR
; Flags - The processor set RFLAGS to the logical-AND of its current value
;         with the complement of the value in IA32_FMASK MSR
;
; The SYSCALL instruction does not save the stack pointer, and the SYSRET
; instruction does not restore it.
;
; SYSRET:
; When SYSRET transfers control to 64-bit mode user code using REX.W, the
; processor gets the privilege level 3 target code segment, instruction pointer,
; stack segment, and flags as follows:
;
; Target Code Segment - Reads a non-NULL selector from IA32_STAR[63:48] + 16
; Target Instruction Pointer - Copies value from RCX into RIP
; Stack Segment - IA32_STAR[63:48] + 8
; EFLAGS - Loaded from R11

; After syscall instruction:
; rdi = arg0
; rsi = arg1
; r10 = arg2 (moved to rcx by your handler)
; r9  = arg3
; r8  = arg4
; rdx = arg5 (for SYSCALL6 only)
syscall_handler:
    push r15                ; store r15 in user stack
    mov r15, rsp            ; save process's stack in r15

    push r11                ; Saved RFLAGS
    push rcx                ; Saved RIP

    pushall                 ; push all registers using macro

    mov rcx, r10

    call [rax * 8 + syscall_funcs]

    popall_syscall          ; Pop all registers except for RAX

    swapgs

    mov rdx, qword [gs:0x0] ; Return errno in rdx
    mov rsp, r15            ; Restore user stack
    pop r15                 ; Pop r15 from user stack

    swapgs

    o64 sysret              ; Return to user mode
                            ; Restores RIP from RCX
                            ; Restores RFLAGS from R11
