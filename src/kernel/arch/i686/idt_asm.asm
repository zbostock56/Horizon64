bits 32

;
; void __attribute__((cdecl)) i686_idt_load(IDT_DESCRIPTOR *);
;
global i686_idt_load
i686_idt_load:
    push ebp
    mov ebp, esp

    ; Load IDT
    mov eax, [ebp + 8]
    lidt [eax]

    mov esp, ebp
    pop ebp
    ret