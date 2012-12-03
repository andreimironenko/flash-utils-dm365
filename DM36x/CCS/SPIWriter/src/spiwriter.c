/* --------------------------------------------------------------------------
    FILE        : spiwriter.c 				                             	 	        
    PROJECT     : DM365 CCS SPI Flashing Utility
    AUTHOR      : Bernie Thompson, based on C672x code by Daniel Allred
    DESC        : Main function for flashing the SPI device on the DM365  
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


/************************************************************
* Local Macro Declarations                                  *
************************************************************/


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
  SPIWRITER_BootObj  bootHeader;

  FILE	*fPtr;
  Uint8	*appPtr;
  Int32	fileSize = 0, headerSize = 0;
  Int8	fileName[256];
  Int8  answer[32];
  
  DEBUG_printString( "Starting DM36x SPIWriter.\r\n");
  
  // Prep device for SPI writing (pinmux/PSC)
  DEVICE_SPIInit(0);
  
  // Initialize SPI Memory Device on SPI0
  hSpiMemInfo = SPI_MEM_open(0);
  if (hSpiMemInfo == NULL)
  {
    DEBUG_printString( "\tERROR: SPI Memory Initialization failed.\r\n" );
    return E_FAIL;
  }
  
  // Calculate header size rounded up to page size
  do
  {
    headerSize += hSpiMemInfo->hMemParams->pageSize;
  }
  while (headerSize < sizeof(SPIWRITER_BootObj));
  
  // Read the UBL file from host
  DEBUG_printString("Enter the binary UBL file name (enter 'none' to skip): \r\n");
  DEBUG_readString(fileName);
  fflush(stdin);

  if (strcmp(fileName,"none") != 0)
  {
    // Open the file from the host
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

    // Erase the SPI flash to accomodate the file size and header
    SPI_MEM_eraseBytes(hSpiMemInfo, DEVICE_SPI_UBL_HDR_OFFSET, fileSize + headerSize );
    SPI_MEM_readBytes(hSpiMemInfo, DEVICE_SPI_UBL_HDR_OFFSET, sizeof(SPIWRITER_BootObj), (Uint8 *)&bootHeader);    
    
    // Prepare the UBL header
    if (hSpiMemInfo->hMemParams->addrWidth == 24)
    {
      bootHeader.magicNum   = UBL_MAGIC_SPI24;
    }
    else if (hSpiMemInfo->hMemParams->addrWidth == 16)
    {
      bootHeader.magicNum   = UBL_MAGIC_SPI16;    
    }
    bootHeader.entryPoint   = 0x00000100;     // default for UBL
    bootHeader.appSize      = fileSize;
    bootHeader.preScalar    = 0x31;           // default prescalar of 50
    bootHeader.fastRead     = 0x1;
    bootHeader.dummy        = 0x0000;
    bootHeader.memAddress   = DEVICE_SPI_UBL_HDR_OFFSET + headerSize;
    bootHeader.ldAddress    = 0x00000020;     // default for UBL
    
    // Write the UBL header
    SPI_MEM_writeBytes(hSpiMemInfo, DEVICE_SPI_UBL_HDR_OFFSET, sizeof(SPIWRITER_BootObj), (Uint8 *)&bootHeader);
    SPI_MEM_readBytes(hSpiMemInfo, DEVICE_SPI_UBL_HDR_OFFSET, sizeof(SPIWRITER_BootObj), (Uint8 *)&bootHeader);    
    
    // Write the UBL to the SPI memory at the specified offset
    SPI_MEM_writeBytes(hSpiMemInfo, bootHeader.memAddress, fileSize, appPtr);
    SPI_MEM_readBytes(hSpiMemInfo, bootHeader.memAddress, fileSize, appPtr);
  }
  
  // Read the App file from host
  DEBUG_printString("Enter the binary application file name (enter 'none' to skip): \r\n");
  DEBUG_readString(fileName);
  fflush(stdin);

  if (strcmp(fileName,"none") != 0)
  {
    // Open the file from the host
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
    
    // Get the entry point and load addresses
    DEBUG_printString("Enter the U-boot or application entry point (in hex): \n");
    DEBUG_readString(answer);
    bootHeader.entryPoint = strtoul(answer, NULL, 16);
    fflush(stdin);

    if ( (bootHeader.entryPoint < DEVICE_DDR2_START_ADDR) || (bootHeader.entryPoint >= DEVICE_DDR2_END_ADDR) )
    {
      DEBUG_printString("\tWARNING: Entry point not in acceptable range - using default 0x81080000.\r\n");
      bootHeader.entryPoint = 0x81080000;
    }
    else
    {
      DEBUG_printString("\tINFO: Selected entry point is ");
      DEBUG_printHexInt(bootHeader.entryPoint);
      DEBUG_printString("\r\n");
    }

    DEBUG_printString("Enter the U-boot or application load address (in hex): \r\n");
    DEBUG_readString(answer);
    bootHeader.ldAddress = strtoul(answer, NULL, 16);

    if ( (bootHeader.ldAddress < DEVICE_DDR2_START_ADDR) || (bootHeader.ldAddress >= DEVICE_DDR2_END_ADDR) )
    {
      DEBUG_printString("\tWARNING: Load address not in acceptable range - using default 0x81080000.\r\n");
      bootHeader.ldAddress = 0x81080000;
    }
    else
    {
      DEBUG_printString("\tINFO: Selected load address is ");
      DEBUG_printHexInt(bootHeader.ldAddress);
      DEBUG_printString("\r\n");
    }    
    
    // Erase the SPI flash to accomodate the file size and header
    SPI_MEM_eraseBytes(hSpiMemInfo, DEVICE_SPI_APP_HDR_OFFSET, fileSize + headerSize );

    // Prepare the APP header
    if (hSpiMemInfo->hMemParams->addrWidth == 24)
    {
      bootHeader.magicNum   = UBL_MAGIC_SPI24;
    }
    else if (hSpiMemInfo->hMemParams->addrWidth == 16)
    {
      bootHeader.magicNum   = UBL_MAGIC_SPI16;    
    }
    bootHeader.appSize      = fileSize;
    bootHeader.preScalar    = 0x00;       // UBL ignores this
    bootHeader.fastRead     = 0x00;       // UBL ignores this
    bootHeader.dummy        = 0x0000;     // UBL ignores this
    bootHeader.memAddress   = DEVICE_SPI_APP_HDR_OFFSET + headerSize;

    // Write the APP header
    SPI_MEM_writeBytes(hSpiMemInfo, DEVICE_SPI_APP_HDR_OFFSET, sizeof(SPIWRITER_BootObj), (Uint8 *) &bootHeader);
    
    // Write the APP to the SPI memory at the specified offset
    SPI_MEM_writeBytes(hSpiMemInfo, bootHeader.memAddress, fileSize, appPtr);
  }  
  
  return E_PASS;
}


/***********************************************************
* End file                                                 *
***********************************************************/
