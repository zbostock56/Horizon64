global smp_trampoline_blob_start
global smp_trampoline_blob_end

smp_trampoline_blob_start:
    incbin "obj/src/sys/smp_base.asm.o"
smp_trampoline_blob_end: