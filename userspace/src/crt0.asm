global _start

extern main
extern exit

_start:
    pop rdi         ; Create a new stack frame
    mov rsi, rsp    ; ^

    call main       ; Jump to the executable code

    mov rax, 23     ; Move the exit system call number (23) into rax
    mov rdi, 0      ; Move the zero (success) exit code into rax
    syscall         ; Call exit

    ret             ; Shouldn't ever be executed