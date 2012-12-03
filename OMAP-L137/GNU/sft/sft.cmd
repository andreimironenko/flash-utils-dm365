stack          0x00008000 /* Stack Size */  
-heap           0x00008000 /* Heap Size */

MEMORY
{
  L2RAM				: origin=0x11800000 length=0x00040000 /* DSP L2 RAM */
  L3RAM				: origin=0x80000000 length=0x00020000 /* Shared L3 RAM */
  DRAM        : origin=0xC0000000 length=0x0E000000 /* DDR RAM */
  DRAM_PROG   : origin=0xCE000000 length=0x01000000 /* DDR for program */
}

SECTIONS
{
  .text       > L3RAM
  .switch			> L3RAM
  .far				> L3RAM
  .const      > L3RAM
  .bss        > L3RAM
  .stack      > L3RAM
  .data       > L3RAM
  .cinit      > L3RAM
  .sysmem     > L3RAM
  .cio        > L3RAM
  .ddr_mem :
  {
    . += 0x00020000;
  } run = L3RAM,type=DSECT, RUN_START(_EXTERNAL_RAM_START), RUN_END(_EXTERNAL_RAM_END)