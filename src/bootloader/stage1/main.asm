; 16 bit output from assembler
bits 16

%define ENDL 0x0D, 0x0A

%define fat12 1
%define fat16 2
%define fat32 3
%define ext2

;
; ---------- FAT Headers ----------
;

section .fsjump
  jmp short start                      ; Boot sector data must start three bytes from beginning
  nop                                  ; So, this code doesn't matter

section .fsheaders
; Boot data bytes (59 in total)
%if (FILESYSTEM == fat12) || (FILESYSTEM == fat16) || (FILESYSTEM == fat32)
  ; BIOS Parameter Block
  bpb_oem:                      db "zbostock"
  bpb_bytes_per_sector:         dw 512
  bpb_sectors_per_cluster:      db 1
  bpb_reserved_sectors:         dw 1
  bpb_fat_count:                db 2
  bpb_dir_entries_count:        dw 0E0h
  bpb_total_sectors:            dw 2880  ; 2880 * 512 = 1.44 MB
  bpb_media_descriptor:         db 0F0h  ; 0xF0 = 3.5" floppy
  bpb_sectors_per_fat:          dw 9
  bpb_sectors_per_track:        dw 18
  bpb_heads:                    dw 2
  bpb_hidden_sectors:           dd 0
  bpb_large_sector_count:       dd 0

  %if (FILESYSTEM == fat32)
    fat32_sectors_per_fat:      dd 0
    fat32_flags:                dw 0
    fat32_fat_version_number:   dw 0
    fat32_rootdir_cluster:      dd 0
    fat32_fsinfo_sector:        dw 0
    fat32_backup_boot_sector:   dw 0
    fat32_reserved:             times 12 db 0
  %endif

  ; Extended Boot Record
  ebr_drive_number:             db 0
                                db 0               ; Implicitly reserved byte
  ebr_signiture:                db 29h             ; Serial number, value doesn't matter
  ebr_volume_id:                db 01h, 02h, 03h, 04h
  ebr_volume_label:             db 'sunrise_bl '   ; Must be 11 bytes
  ebr_system_id:                db 'FAT32   '      ; Must be 8 bytes 
%endif
; ------------------------------


;
; Entry point from BIOS
;
section .entry
start:
  ; Move partition entry from MBR to a different location so we don't
  ; overwrite it
  mov ax, PARTITION_ENTRY_SEGMENT
  mov es, ax
  mov di, PARTITION_ENTRY_OFFSET
  mov cx, 16
  rep movsb

  ; Set up data segments
  mov ax, 0
  mov ds, ax
  mov es, ax

  ; Set up stack pointer and stack segment
  mov ss, ax
  mov sp, 0x7C00

  ; Fixing the oddities with some BIOSes loading
  ; 07C0:0000 instead of 0000:7C00
  push es
  push word .after
  retf

.after:
  ; Read drive information (sectors per track and head count)
  ; from BIOS rather than relying on potentially corrupted
  ; disk image
  mov [ebr_drive_number], dl

  ; Check if extensions present
  mov ah, 0x41
  mov bx, 0x55AA
  stc
  int 13h

  jc .no_disk_extensions
  cmp bx, 0xAA55
  jne .no_disk_extensions

  ; Extensions are present
  mov byte [have_extensions], 1
  jmp .after_disk_extensions_check

.no_disk_extensions:
  mov byte [have_extensions], 0
.after_disk_extensions_check:
  ; Load stage2.bin
  mov si, stage2_location

  mov ax, STAGE2_LOAD_SEGMENT
  mov es, ax

  mov bx, STAGE2_LOAD_OFFSET
.loop:
  mov eax, [si]
  add si, 4
  mov cl, [si]
  inc si

  cmp eax, 0
  je .read_finish

  call disk_read

  xor ch, ch
  shl cx, 5
  mov di, es
  mov di, cx
  mov es, di

  jmp .loop

.read_finish:
  ; Jump to stage2.bin
  mov dl, [ebr_drive_number]
  mov si, PARTITION_ENTRY_OFFSET
  mov di, PARTITION_ENTRY_SEGMENT

  mov ax, STAGE2_LOAD_SEGMENT
  mov ds, ax
  mov es, ax

  jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

  cli
  hlt
;
; ---- END START FUNCTION ----
;


section .text
;
; Helper Routines
;

; ---------- print ----------
; Output character(s) to screen
; Input: ds:si

print:
  push si
  push ax
  push bx

.loop:
  lodsb         ; Loads character
  or al, al     ; Is the character null?
  jz .endloop

  mov ah, 0x0e  ; Call bios interrupt
  mov bh, 0     ; Set page number to zero
  int 0x10

  jmp .loop

.endloop:
  pop bx
  pop ax
  pop si
  ret
; --------------------------

;
; Disk I/O Routines
;

; ---------- lba_to_chs ----------
; Output: 
;   - cx [bits 0 - 5]: sector number
;   - cx [bits 6 - 15]: cylinder
;   - dh: head
; Input: 
;   - LBA address (ax) 
; Desc:
;   Sector = (LBA % sectors_per_track) + 1
;   Head = (LBA / sectors_per_track) % heads
;   cylinder = (LBA / sectors_per_track) / heads

lba_to_chs:
  push ax
  push dx

  xor dx, dx                        ; Set dx to zero
  div word [bpb_sectors_per_track]  ; ax = LBA / sectors_per_track
                                    ; dx = remainer of division

  inc dx                            ; Perform increment as described in desc
  mov cx, dx

  xor dx, dx                        ; Set dx to zero
  div word [bpb_heads]              ; ax = (LBA / sectors_per_track) / heads = cylinder
                                    ; dx = (LBA / sectors_per_track) % heads = head
  mov dh, dl                        ; move lower bits of ax (signified by dl)
  mov ch, al                        ; ch = cylinder (lower 8 bits)
                                    ; Next three operations set bits as described in desc
  shl ah, 6
  or cl, ah

  pop ax
  mov dl, al                        ; Restore dl
  pop ax
  ret
; --------------------------------

; ---------- disk_read ----------
; Input:
;   - ax: LBA address
;   - cl: Number of sectors to read (up to 128)
;   - dl: Drive number
;   - es:bx: Location to store the data read

disk_read:
  push eax
  push bx
  push cx
  push dx
  push si
  push di


  cmp byte [have_extensions], 1
  jne .no_disk_extensions

  ; With extensions
  mov [extensions_dap.lba_to_chs], eax
  mov [extensions_dap.segment], es
  mov [extensions_dap.offset], bx
  mov [extensions_dap.count], cl

  mov ah, 0x42
  mov si, extensions_dap
  mov di, 3
  jmp .retry

.no_disk_extensions:
  push cx                              ; temporarily save cl (number of sectors to read)
  call lba_to_chs                      ; compute chs
  pop ax                               ; al = number of sectors to read

  mov ah, 02h
  mov di, 3                            ; retry count

.retry:
  pusha                                ; save all registers, can;t be certain what the BIOS changes
  stc
  int 13h
  jnc .success

  ; read failed, reset the disk
  popa
  call disk_reset

  dec di
  test di, di 
  jnz .retry

.fail:
  ; All attempts are exhausted
  jmp floppy_error

.success:
  popa

  pop di
  pop si
  pop dx
  pop cx
  pop bx
  pop eax
  ret
; -------------------------------

; --------- disk_reset ----------
;
; Resets disk controller
; Parameters:
;   dl: drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret
; -------------------------------


;
; Error Handlers
;
floppy_error:
  mov si, read_failed_msg
  call print
  jmp wait_key_and_reboot

wait_key_and_reboot:
  mov ah, 0
  int 16h                     ; Await keypress
  jmp 0FFFFh:0                ; Jump to beginning of bios (should reboot system)

stage2_not_found:
  mov si, stage2_not_found_msg
  call print
  jmp wait_key_and_reboot


section .rodata
; Message String Constants
read_failed_msg:             db 'Read error!', ENDL, 0
stage2_not_found_msg:        db 'STAGE2.BIN not found!', ENDL, 0
stage2_bin_fn:               db 'STAGE2  BIN'

section .data
  have_extensions:           db 0
  extensions_dap:
    .size                    db 10h
                             db 0
    .count                   dw 0
    .offset                  dw 0
    .segment                 dw 0
    .lba_to_chs              dq 0

STAGE2_LOAD_SEGMENT          equ 0x0
STAGE2_LOAD_OFFSET           equ 0x500

PARTITION_ENTRY_SEGMENT      equ 0x2000
PARTITION_ENTRY_OFFSET       equ 0x0

section .data
  global stage2_location
  stage2_location:           times 30 db 0

section .bss
buffer:                      resb 512