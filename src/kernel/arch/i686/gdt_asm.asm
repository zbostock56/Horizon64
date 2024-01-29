bits 32

;
; void __attribute__((cdecl)) i686_gdt_load(GDT_DESCRIPTOR *, uint16_t, uint16_t);
;
; Both uint16_t's are the code and data segments to load. These can either be ring0
; or ring3 segements to alter the privilage level that the CPU is running in. For startup
; both segments are set to ring0 mode.
;

global i686_gdt_load
i686_gdt_load:
    push ebp
    mov ebp, esp

    ; Load GDT
    mov eax, [ebp + 8]
    lgdt [eax]

    ; Reload code segement
    ;
    ; Code segment cannot be reloaded without performing a
    ; far jump
    ;
    ; Push the descriptor address on the stack and then perform
    ; a far jump to reload the segment
    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf

.reload_cs:

    ; Reload data segments
    ; (these can all be loaded manually)
    mov ax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, ebp
    pop ebp
    ret