%macro x86_enter_real_mode 0
  bits 32
  jmp word 18h:.pmode16                ; 1 - jump to 16-bit protected mode segment

.pmode16:
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

;
; x86_disk_reset(uint8_t drive)
;
global x86_disk_reset
x86_disk_reset:
  bits 32

  push ebp
  mov ebp, esp

  x86_enter_real_mode
  bits 16

  mov ah, 0

  ; Setup arguments for BIOS interrupt
  mov dl, [bp + 8]                     ; Drive number = dl

  ; Call BIOS interrupt
  mov ah, 0
  stc
  int 13h                              ; Call BIOS interrupt

  ; Set return value
  mov eax, 1
  sbb eax, 0                            ; 1 on success, 0 on fail, ignoring return from BIOS

  push eax                              ; eax is clobbered on upon changing CPU modes, push to save value

  x86_enter_protected_mode
  bits 32
  pop eax

  mov sp, bp
  pop ebp
  ret

;
; x86_disk_read(uint8_t drive, uint16_t cylinder, uint16_t head,
;               uint16_t sector, uint8_t count, uint8_t far *data_out)
;
global x86_disk_read
x86_disk_read:
  bits 32
  push ebp
  mov ebp, esp

  x86_enter_real_mode
  bits 16

  push ebx
  push es

  ; Setup arguments for BIOS interrupt
  mov dl, [bp + 8]                     ; Drive number = dl

  mov ch, [bp + 12]                     ; ch = cylinder (lower 8 bits)
  mov cl, [bp + 13]                     ; cl = cylinder (bits 6-7)
  shl cl, 6

  mov al, [bp + 16]
  and al, 3Fh                          ; apply mask to zero out bits
  or cl, al                            ; cl = sector number 1 through 63 (bits 0-5)

  mov dh, [bp + 20]                     ; dh = head

  mov al, [bp + 24]                    ; Number of sectors to read (must be non-zero)

  linear_to_seg_offset [bp + 28], es, ebx, bx

  ; Call BIOS interupt
  mov ah, 02h
  stc
  int 13h

  ; Set return value
  mov eax, 1
  sbb eax, 0                            ; 1 on success, 0 on fail, ignoring return from BIOS

  pop es
  pop ebx

  push eax

  x86_enter_protected_mode
  bits 32

  pop eax

  mov esp, ebp
  pop ebp
  ret

;
; x86_disk_get_drive_params(uint8_t drive, uint8_t *drive_type_out,
;                           uint16_t *cylinders_out, uint16_t *sectors_out,
;                           uint16_t *heads_out)
;
global x86_disk_get_drive_params
x86_disk_get_drive_params:
  bits 32

  push ebp
  mov ebp, esp

  x86_enter_real_mode
  bits 16

  ; Save adjusted registers
  push es
  push di
  push si
  push esi

  mov dl, [bp + 8]                     ; dl = disk drive
  mov ah, 08h
  mov di, 0                            ; es:di - 0000:0000
  mov es, di
  stc
  int 13h

  ; Return value of function
  mov eax, 1
  sbb eax, 0

  ; Set passed in pointers from arguments
  linear_to_seg_offset [bp + 12], es, esi, si
  mov [es:si], bl

  ; Cylinders
  mov bl, ch                           ; cylinders, lower bits in ch
  mov bh, cl                           ; cylinders, upper bits in cl (6-7)
  shr bh, 6
  inc bx

  linear_to_seg_offset [bp + 16], es, esi, si
  mov [es:si], bx

  ; Sectors
  xor ch, ch
  and cl, 3Fh                          ; apply bitmask

  linear_to_seg_offset [bp + 20], es, esi, si
  mov [es:si], cx

  ; Heads
  mov cl, dh                           ; heads = dh
  inc cx

  linear_to_seg_offset [bp + 24], es, esi, si
  mov [es:si], cx


  ; Restore saved registers
  pop esi
  pop si
  pop di
  pop es

  push eax

  x86_enter_protected_mode
  bits 32
  pop eax

  mov esp, ebp
  pop ebp
  ret


global x86_panic
x86_panic:
  x86_enter_real_mode
  bits 16

  mov ah, 0
  int 16h                     ; Await keypress
  jmp 0FFFFh:0                ; Jump to beginning of bios (should reboot system)
  ; Should never get to this point
  cli
  hlt

;
; int x86_E820GetNextBlock(E820_MEMORY_BLOCK *, uint32_t *);
;

E820_signature  equ 0x534D4150

global x86_E820_get_next_block
x86_E820_get_next_block:
  bits 32
  push ebp
  mov ebp, esp

  x86_enter_real_mode
  bits 16

  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ds
  push es

  ; Setup parameters
  linear_to_seg_offset [bp + 8], es, edi, di     ; es-di pointer to struct

  linear_to_seg_offset [bp + 12], ds, esi, si    ; ebx = pointer to continuation_id
  mov ebx, ds:[si]

  mov eax, 0xE820                                ; eax - function "pointer"
  mov edx, E820_signature                         ; edx - signiture (magic number)
  mov ecx, 24                                    ; ecx - size of structure to read

  ; Call BIOS function
  int 0x15

  cmp eax, E820_signature
  jne .error

  ; Should be successful if BIOS supports this call
.success:
  mov eax, ecx                                   ; Return size of block
  mov ds:[si], ebx                               ; Fill structure to return to caller
  jmp .end

.error:
  mov eax, -1

.end:

  pop es
  pop ds
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx

  push eax
  x86_enter_protected_mode
  bits 32
  pop eax

  mov esp, ebp
  pop ebp
  ret