-stack          0x00008000 /* Stack Size */  
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
  .text       > L2RAM
  .switch			> L2RAM
  .far				> L2RAM
  .const      > L2RAM
  .bss        > L2RAM
  .stack      > L2RAM
  .data       > L2RAM
  .cinit      > L2RAM
  .sysmem     > L2RAM
  .cio        > L2RAM
  .ddr_mem :
  {
    . += 0x00020000;
  } run = L3RAM,type=DSECT, RUN_START(_EXTERNAL_RAM_START), RUN_END(_EXTERNAL_RAM_END)
}