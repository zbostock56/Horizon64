%macro x86_enter_real_mode 0
  bits 32
  jmp word 18h:.pmode16                ; 1 - jump to 16-bit protected mode segment

.pmode16
  bits 16
  ; 2 - disable protected mode bit in cr0
  mov eax, cr0 
  and al, ~1
  mov cr0, eax

  ; 3 - jump to real mode
  jmp word 00h:.rmode 

.rmode:
  ; 4 - setup segments
  mov ax, 0
  mov ds, ax
  mov ss, ax

  ; 5 - enable interrupts
  sti
%endmacro

%macro x86_enter_protected_mode 0
  cli

  ; 4 - set protection enable flag in CR0
  mov eax, cr0 
  or al, 1
  mov cr0, eax

  ; 5 - far jump into protected mode
  jmp dword 08h:.pmode

.pmode:
  bits 32

  ; 6 - setup segment registers
  mov ax, 0x10
  mov ds, ax
  mov ss, ax
%endmacro

%macro linear_to_seg_offset 4
  mov %3, %1                          ; linear address to eax
  shr %3, 4
  mov %2, %4
  mov %3, %1                          ; linear address to eax
  and %3, 0xF
%endmacro

global x86_outb
x86_outb:
  bits 32
  mov dx, [esp + 4]
  mov al, [esp + 8]
  out dx, al
  ret

global x86_inb
x86_inb:
  bits 32
  mov dx, [esp + 4]
  xor eax, eax
  in al, dx
  ret
