/* --------------------------------------------------------------------------
  FILE        : spiwriter.h
  PROJECT     : TI Booting and Flashing Utilities
  AUTHOR      : Daniel Allred
  DESC        : Header file for the SPIWriter application for flashing the 
                DM36x EVM from Spectrum Digital.
 ----------------------------------------------------------------------------- */

#ifndef _SPIWRITER_H_
#define _SPIWRITER_H_

#include "tistdtypes.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

/**************************************************
* Global Macro Declarations                       *
**************************************************/

//UBL version number
#define UBL_VERSION_STRING "1.00"
#ifdef UBL_NAND
#define UBL_FLASH_TYPE "NAND"
#endif

// Define MagicNumber constants
#define MAGIC_NUMBER_VALID          (0xA1ACED00)



// Used by RBL when doing NAND boot
#define UBL_MAGIC_SPI16             (0xA1ACED00)		/* 16-bit EEPROM */
#define UBL_MAGIC_SPI24             (0xA1ACED01)		/* 24-bit Flash */

// Used by UBL when doing UART boot, UBL Nor Boot, or NAND boot
#define UBL_MAGIC_BIN_IMG           (0xA1ACED66)		/* Execute in place supported*/

// Define max UBL image size
#define UBL_IMAGE_SIZE              (0x00003800u)

// Define max app image size
#define APP_IMAGE_SIZE              (0x02000000u)


/************************************************
* Global Typedef declarations                   *
************************************************/

typedef struct
{
  Uint32 magicNum;    // Expected magic number
  Uint32 entryPoint;  // Entry point of the user application
  Uint32 ublSize;     // Number of pages where boot loader is stored
  Uint8  preScalar;   // SPI divider
  Uint8  fastRead;    // Enables sequential read ops on SPI memory
  Uint16 dummy;       // Starting block number where User boot loader is stored
  Uint32 memAddr;     // SPI memory offset where UBL image is located
  Uint32 loadAddr;    // Address where image is copied to
}
SPIWRITER_Boot;


/******************************************************
* Global Function Declarations                        *
******************************************************/

void main(void);


/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif

#endif //_SPIWRITER_H_




