global i686_panic
i686_panic:
    cli
    hlt

global i686_outb
i686_outb:
  bits 32
  mov dx, [esp + 4]
  mov al, [esp + 8]
  out dx, al
  ret

global i686_inb
i686_inb:
  bits 32
  mov dx, [esp + 4]
  xor eax, eax
  in al, dx
  ret

global i686_enable_interrupts
i686_enable_interrupts:
    sti
    ret

global i686_disable_interrupts
i686_disable_interrupts:
    cli
    ret