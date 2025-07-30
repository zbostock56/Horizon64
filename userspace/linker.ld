ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(4);    /* R-X */
    data PT_LOAD FLAGS(6);    /* RW- */
}

SECTIONS
{
    .text 0xFFFF800040000000 : ALIGN(4096)
    {
        *(.text)
        . = ALIGN(4096);
    } :text

    .rodata : ALIGN(4096)
    {
        *(.rodata)
        . = ALIGN(4096);
    } :text  /* Still part of the read-only segment */

    .data : ALIGN(4096)
    {
        *(.data)
        . = ALIGN(4096);
    } :data

    .bss : ALIGN(4096)
    {
        *(.bss)
        . = ALIGN(4096);
    } :data
}