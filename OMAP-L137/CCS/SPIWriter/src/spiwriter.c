/* --------------------------------------------------------------------------
    FILE        : spiwriter.c 				                             	 	        
    PROJECT     : OMAP-L137 CCS SPI Flashing Utility
    AUTHOR      : Daniel Allred
    DESC        : Main function for flashing the SPI device on the OMAP-L137  
 ----------------------------------------------------------------------------- */

// C standard I/O library
#include "stdio.h"

// General type include
#include "tistdtypes.h"

// Device specific CSL
#include "device.h"

// Misc. utility function include
#include "util.h"

// Debug module
#include "debug.h"

// SPI memory driver include
#include "spi_mem.h"
#include "device_spi.h"


// This module's header file 
#include "spiwriter.h"


/************************************************************
* Explicit External Declarations                            *
************************************************************/

extern cregister volatile unsigned int TSCH;
extern cregister volatile unsigned int TSCL;


/************************************************************
* Local Macro Declarations                                  *
************************************************************/

#define SPI_PERIPHNUM		(0)

/************************************************************
* Local Typedef Declarations                                *
************************************************************/


/************************************************************
* Local Function Declarations                               *
************************************************************/

static Uint32 spiwriter(void);


/************************************************************
* Local Variable Definitions                                *
************************************************************/


/************************************************************
* Global Variable Definitions
************************************************************/


/************************************************************
* Global Function Definitions                               *
************************************************************/

void main( void )
{
  Uint32 status;

  // Init memory alloc pointer to start of DDR heap
  UTIL_setCurrMemPtr(0);

  // System init
  if (DEVICE_init() !=E_PASS)
  {
    exit();
  }

  // Execute the SPI flashing
  status = spiwriter();

  if (status != E_PASS)
  {
    DEBUG_printString("\tSPI flashing failed!\r\n");
  }
  else
  {
    DEBUG_printString("\tSPI boot preparation was successful!\r\n" );
  }
}


/************************************************************
* Local Function Definitions                                *
************************************************************/

static Uint32 spiwriter()
{
  SPI_MEM_InfoHandle hSpiMemInfo;

  FILE	*fPtr;
  Uint8	*appPtr;
  Int32	fileSize = 0;
  Int8	fileName[256];
  Uint32  baseAddress = 0;

  DEBUG_printString( "Starting OMAP-L137 SPIWriter.\r\n");
  
  // Prep device for SPI writing (pinmux/PSC)
  DEVICE_SPIInit(SPI_PERIPHNUM);
  
  // Initialize SPI Memory Device on SPI0
  hSpiMemInfo = SPI_MEM_open(SPI_PERIPHNUM);
  if (hSpiMemInfo == NULL)
  {
    DEBUG_printString( "\tERROR: SPI Memory Initialization failed.\r\n" );
    return E_FAIL;
  }

  SPI_MEM_readBytes( hSpiMemInfo, 0x00, 128, (Uint8 *)fileName);

  // Read the AIS file from host
  DEBUG_printString("Enter the binary AIS application file name (enter 'none' to skip): \r\n");
  DEBUG_readString(fileName);
  fflush(stdin);

  if (strcmp(fileName,"none") != 0)
  {
    // Open an File from the hard drive
    fPtr = fopen(fileName, "rb");
    if(fPtr == NULL)
    {
      DEBUG_printString("\tERROR: File ");
      DEBUG_printString(fileName);
      DEBUG_printString(" open failed.\r\n");
      return E_FAIL;
    }

    // Initialize the pointer
    fileSize = 0;

    // Read file size
    fseek(fPtr,0,SEEK_END);
    fileSize = ftell(fPtr);

    // Check to make sure image was read correctly
    if(fileSize == 0)
    {
      DEBUG_printString("\tERROR: File read failed.\r\n");
      fclose (fPtr);
      return E_FAIL;
    }
    // Check to make sure the app image will fit 
    else if ( fileSize > hSpiMemInfo->hMemParams->memorySize )
    {
      DEBUG_printString("\tERROR: File too big.. Closing program.\r\n");
      fclose (fPtr);
      exit(0);
    }

    // Setup pointer in RAM
    appPtr = (Uint8 *) UTIL_allocMem(fileSize);

    fseek(fPtr,0,SEEK_SET);

    if (fileSize != fread(appPtr, 1, fileSize, fPtr))
    {
      DEBUG_printString("\tWARNING: File Size mismatch.\r\n");
    }

    fclose (fPtr);
    
    DEBUG_printString("\tINFO: File read complete.\r\n");

    // Erase the SPI flash to accomodate the file size
    if (SPI_MEM_eraseBytes( hSpiMemInfo, 0x00, fileSize ) != E_PASS)
    {
      DEBUG_printString("\tERROR: Erasing SPI failed.\r\n");
      return E_FAIL;
    }

	SPI_MEM_readBytes( hSpiMemInfo, 0x00, 128, appPtr);

    // Write the application data to the flash (32 bytes at a time)
    if (SPI_MEM_writeBytes( hSpiMemInfo, 0x00, fileSize, appPtr) != E_PASS)
    {
      DEBUG_printString("\tERROR: Writing SPI failed.\r\n");
      return E_FAIL;
    }

	SPI_MEM_readBytes( hSpiMemInfo, baseAddress, fileSize, appPtr);
  }
  return E_PASS;
}


/***********************************************************
* End file                                                 *
***********************************************************/
