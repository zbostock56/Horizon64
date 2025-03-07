bits 64
section .text

%include "src/sys/asm_macros.asm"

extern isr_handler

; Denote as global so isr_disp appears in kernel.map file for backtrace info
global isr_disp

isr_disp:
  pushall
  mov rdi, rsp      ; Push the stack pointer on as first argument
  call isr_handler  ; Call general handler
  popall
  add rsp, 16       ; Pop off the error code and interrupt number
  iretq

%macro ISR_NOERRORCODE 1
global ISR%1
ISR%1:
  push 0            ; Push dummy error code
  push %1           ; Push interrupt number
  jmp isr_disp      ; Call general dispatcher
%endmacro

%macro ISR_ERRORCODE 1
global ISR%1
ISR%1:
                    ; Error code is already on the stack
  push %1           ; Push interrupt number
  jmp isr_disp      ; Call general dispatcher
%endmacro

%include "include/sys/interrupts/isrs_gen.inc"