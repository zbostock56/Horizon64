extern apic_send_end_of_interrupt

global dummy_apic_timer_handler

dummy_apic_timer_handler:
    push rbp
    mov rbp, rsp

    push rax
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push r10
    push r11

    call apic_send_end_of_interrupt

    pop r11
    pop r10
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax

    pop rbp

    iretq