/* --------------------------------------------------------------------------
  FILE        : spiboot.h
  PROJECT     : TI Booting and Flashing Utilities
  AUTHOR      : Daniel Allred
  DESC        : This file defines all needed structures and macros for 
                booting a user application in SPI memory mode.
 ----------------------------------------------------------------------------- */

#ifndef _SPIBOOT_H_
#define _SPIBOOT_H_

#include "tistdtypes.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

/************************************************************
* Global Macro Declarations                                 *
************************************************************/


/************************************************************
* Global Typedef declarations                               *
************************************************************/

typedef struct _SPIBOOT_HEADER_
{
  Uint32 magicNum;    // Expected magic number
  Uint32 entryPoint;  // Entry point of the user application
  Uint32 appSize;     // Number of pages where boot loader is stored
  Uint32 dummy;       // Unused for UBL loading application
  Uint32 memAddress;  // SPI memory offset where application image is located
  Uint32 ldAddress;   // Address where image is copied to
}
SPIBOOT_HeaderObj,*SPIBOOT_HeaderHandle;


/******************************************************
* Global Function Declarations                        *
******************************************************/

extern __FAR__ Uint32 SPIBOOT_copy(void);


/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif

#endif //_SPIBOOT_H_




