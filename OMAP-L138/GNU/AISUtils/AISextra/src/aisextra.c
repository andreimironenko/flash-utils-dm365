/* --------------------------------------------------------------------------
    FILE        : aisxtra.c 				                             	 	        
    PURPOSE     : AIS Extra commands for use in AIS Scripts
    PROJECT     : TI Boot and Flash Utilities
    AUTHOR      : Daniel Allred
 ----------------------------------------------------------------------------- */

#include "tistdtypes.h"
 
#include "device.h"
 
#pragma FUNC_EXT_CALLED(MakeROMEmulatableAndWait)
#pragma CODE_SECTION(MakeROMEmulatableAndWait,".text")
void MakeROMEmulatableAndWait()
{
  VUint32 *ptr;
	Uint32 i;
  
  // Update all L2 memory to be emulatable
  for (i=0; i<32; i++)
  {
    MPPACTL->L1DMPPA[i] |= (0x40);
  }
  
  // Update all L2 memory to be emulatable
  for (i=0; i<32; i++)
  {
    MPPACTL->L1PMPPA[i] |= (0x40);
  }
   
  // Update all L2 memory to be emulatable
  for (i=0; i<64; i++)
  {
    MPPACTL->L2MPPA[i] |= (0x40);
  }	
  
  // Enable all TAPs
  SECURITYCTL->SYSCONTROLPROTECT = 0x00020000;    // Unlock MMRs
  SECURITYCTL->SYSTAPEN = 0xFFFFFFFF;             // Enable all TAPs
  
  // Initialize inhibitSKLoad flag
  // as a workaround for ROM bug
  ptr = (VUint32 *)0x00805488;
  *ptr = 0x00000000;
  
  // Invalidate caches
  L1DCTL->L1DINV = 0x01;
  L1PCTL->L1PINV = 0x01;
}

