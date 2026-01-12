/*
// =============================================================================
//  Program : test_dsa.ld
//  Author  : DSA Test linker script
//  Date    : Dec/25/2024
// -----------------------------------------------------------------------------
//  Description:
//  簡化版 linker script，用於小型 DSA 測試程式。
// =============================================================================
*/

__stack_size = 0x800;
__heap_size  = 0x10000;

MEMORY
{
    code_ram   (rx!rw) : ORIGIN = 0x80001000, LENGTH = 0x8000
    data_ram   (rw!x)  : ORIGIN = 0x80009000, LENGTH = 0x20000
}

ENTRY(crt0)

SECTIONS
{
    .text :
    {
        *(.text*)
    } > code_ram

    .data :
    {
        *(.data)
        *(.bss)
        *(.rodata*)
    } > data_ram

    .heap : ALIGN(0x10)
    {
        __heap_start = .;
        . += __heap_size;
    } > data_ram

    .stack : ALIGN(0x10)
    {
        . += __stack_size;
        __stack_top = .;
    } > data_ram
}
