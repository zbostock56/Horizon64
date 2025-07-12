%include "src/sys/asm_macros.asm"

global enter_ctxsw
global exit_ctxsw
global force_ctxsw
global fork_ctxsw

extern ctxsw

;
;   void enter_ctxsw(void *v)
;
enter_ctxsw:
    cli                     ; Disable interrupts

    pushall                 ; macro

    mov rdi, rsp            ; Argument 1 (rdi) = Stack pointer (rsp)
    mov rsi, 0              ; Argument 2 (rsi) = 0 (Denotes to scheduler
                            ;                       this is normal ctxsw)

    call ctxsw              ; Call C-based handler

    ; Should not go here
    add rsp, 120
    iretq

;
;   void exit_ctxsw(void *next_stack, uint64_t cr3val)
;       Argument 1: (next_stack) Stack pointer of the next process to run
;       Argument 2: (cr3val) CR3 value if it needs to be changed
;
exit_ctxsw:
    ; Have to set Control Register 3 (CR3) here
    test rsi, rsi           ; Check if 2nd argument is 0 (a.k.a pnext->addrspace
                            ;                             is NULL)
    jz .dont_load_cr3
    mov cr3, rsi            ; 2nd arg populated, use it to set CR3

.dont_load_cr3:
    mov rsp, rdi            ; Move the stack pointer to the next process
    popall
    iretq

;
;   void force_ctxsw()
;
force_ctxsw:
    cli

    mov rax, rsp

    push qword 0x30         ; DS (Data segment)
    push rax
    push qword 0x202        ; RFLAGS
    push qword 0x28         ; CS (Code segment)
    mov rax, .exit
    push rax

    pushall

    mov rdi, rsp            ; Argument 1 (rdi) = Stack pointer (rsp)
    mov rsi, 1              ; Argument 2 (rsi) = 1 (Denotes to scheduler
                            ;                       this is forced ctxsw)

    call ctxsw

    add rsp, 120
    iretq

.exit:
    ret


;
;   void fork_ctxsw()
;
fork_ctxsw:
    cli

    mov rax, rsp

    push qword 0x30         ; DS (Data segment)
    push rax
    push qword 0x202        ; RFLAGS
    push qword 0x28         ; CS (Code segment)
    mov rax, .exit
    push rax

    pushall

    mov rdi, rsp            ; Argument 1 (rdi) = Stack pointer (rsp)
    mov rsi, 2              ; Argument 2 (rsi) = 2 (Denotes to scheduler
                            ;                       this is fork ctxsw)

    call ctxsw

    add rsp, 120
    iretq

.exit:
    ret

