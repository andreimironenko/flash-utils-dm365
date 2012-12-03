-lrts64plus.lib
-stack          0x00000800 /* Stack Size */  
-heap           0x00000800 /* Heap Size */

MEMORY
{
  DRAM        org=0xC0000000 len=0x04000000 /* SDRAM */
  L2RAM       org=0x11800000 len=0x00040000 /* DSP L2RAM */  
  SHARED_RAM  org=0x80000000 len=0x00020000 /* DDR for program */
  AEMIF       org=0x60000000 len=0x02000000 /* AEMIF CS2 region */
  AEMIF_CS3   org=0x62000000 len=0x02000000 /* AEMIF CS3 region */
}

SECTIONS
{
  .text :
  {
  } > L2RAM
  .const :
  {
  } > L2RAM
  .bss :
  {
  } > L2RAM
  .far :
  {
  } > L2RAM
  .stack :
  {
  } > L2RAM
  .data :
  {
  } > L2RAM
  .cinit :
  {
  } > L2RAM
  .sysmem :
  {
  } > L2RAM
  .cio :
  {
  } > L2RAM
  .switch :
  {
  } > L2RAM
  .aemif_mem :
  {
  } > AEMIF_CS3, RUN_START(_NANDStart)
  .ddrram	 :
  {
    . += 0x00020000;
  } > SHARED_RAM, type=DSECT, START(_EXTERNAL_RAM_START), END(_EXTERNAL_RAM_END)
}