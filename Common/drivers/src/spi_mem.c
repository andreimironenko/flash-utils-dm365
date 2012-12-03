/* --------------------------------------------------------------------------
  FILE      : spi_mem.c
  PROJECT   : Catalog Boot-Loader and Flasher Utilities
  AUTHOR    : Daniel Allred
  DESC      : Generic SPI memory driver file
-------------------------------------------------------------------------- */

// General type include
#include "tistdtypes.h"

// Device specific CSL
#include "device.h"

// Util functions
#include "util.h"

// SPI module's header file 
#include "spi.h"

// Platform/device specific SPI info
#include "device_spi.h"

// This module's header file
#include "spi_mem.h"


/************************************************************
* Explicit External Declarations                            *
************************************************************/


/************************************************************
* Local Macro Declarations                                  *
************************************************************/


/************************************************************
* Local Function Declarations                               *
************************************************************/

static Uint32 LOCAL_xferCmdAddrBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint8 command, Uint32 addr);

#ifndef USE_IN_ROM
  static Uint32 LOCAL_waitForReady(SPI_MEM_InfoHandle hSPIMemInfo);
  static void LOCAL_issueWRENCommand(SPI_MEM_InfoHandle hSPIMemInfo);
  static void LOCAL_SPIFlash_bulkErase(SPI_MEM_InfoHandle hSPIMemInfo);
  static void LOCAL_SPIFlash_blockErase(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 blockAddr);
  static void LOCAL_SPIFlash_sectorErase(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 sectorAddr);
#endif


/************************************************************
* Local Variable Definitions                                *
************************************************************/


/************************************************************
* Global Variable Definitions                               *
************************************************************/

// Defualt 

#ifdef USE_IN_ROM
  SPI_MEM_InfoObj     gSPIMemInfo;
  SPI_MEM_ParamsObj   gSPIMemParams;
#endif


/************************************************************
* Global Function Definitions                               *
************************************************************/

// Initialze SPI interface
SPI_MEM_InfoHandle SPI_MEM_open(Uint32 spiPeripheralNum)
{
  Uint8 spibuf;
  SPI_ConfigHandle hSpiConfig = NULL;
  SPI_MEM_InfoHandle hSPIMemInfo = NULL;
     
#ifdef USE_IN_ROM
  hSPIMemInfo = (SPI_MEM_InfoHandle) &gSPIMemInfo;
#else
  hSPIMemInfo = (SPI_MEM_InfoHandle) UTIL_allocMem(sizeof(SPI_MEM_InfoObj));
#endif

  // If no device-specific SPI config provided, use default
  hSpiConfig = (SPI_ConfigHandle) (hDEVICE_SPI_config == NULL) ? hDEFAULT_SPI_CONFIG : hDEVICE_SPI_config;

  // Open SPI peripheral
  hSPIMemInfo->hSPIInfo = SPI_open
  (
    spiPeripheralNum,
    SPI_ROLE_MASTER,
    SPI_MODE_3PIN,
    hSpiConfig
  );  
  
  if (hDEVICE_SPI_MEM_params != NULL)
  {
    hSPIMemInfo->hMemParams = hDEVICE_SPI_MEM_params;
  }
  else
  {
    Uint8 defaultBusVal = 0x22;
#ifdef USE_IN_ROM
    hSPIMemInfo->hMemParams = (SPI_MEM_ParamsHandle) &gSPIMemParams;
#else
    hSPIMemInfo->hMemParams = (SPI_MEM_ParamsHandle) UTIL_allocMem(sizeof(SPI_MEM_ParamsObj));
#endif
  
    // Assert chip select
    SPI_enableCS(hSPIMemInfo->hSPIInfo);

    // Send memory read command
    defaultBusVal = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_READ);

    // Send 8-bit adresss, receive dummy (default bus value
    SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

    // Receive data from 8-bit device OR
    // Transmit next part of 16-bit address and receive dummy
    spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

    // Check for 8-bit memory
    if (spibuf != defaultBusVal)
    {
      hSPIMemInfo->hMemParams->addrWidth = 8;
    }
    else
    {
      // Receive data from 16-bit device OR
      // Transmit next part of 24-bit address and receive dummy
      spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

      // Check for 16-bit memory
      if (spibuf != defaultBusVal)
        hSPIMemInfo->hMemParams->addrWidth = 16;
      else
      {
        // Receive data from 24-bit device OR
        // Transmit dummy
        spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

        // Check for 24-bit memory
        if (spibuf != defaultBusVal)
          hSPIMemInfo->hMemParams->addrWidth = 24;
        else
        {
          DEBUG_printString( "Warning: no response to SPI memory address width detection, using default.\r\n");
          // Assume Atmel devices
		      #ifdef SPI_WRITER_DM365
		      DEBUG_printString( "Now defaulting to 24 bit addressing for DM365 SPI flash.\r\n");
		      hSPIMemInfo->hMemParams->addrWidth = 24;
		      #endif
        }
      }
    }

    // De-assert chip select
    SPI_disableCS(hSPIMemInfo->hSPIInfo);

    // Try to determine if this is flash or EEPROM by
    // issuing a DEVICE ID Read command

    // Assert chip select
    SPI_enableCS(hSPIMemInfo->hSPIInfo);

    // Send memory read command
    SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_JEDEC_ID);

    // Send dummy data, receive manufacture ID
    spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

    if (spibuf != defaultBusVal)
    {
      hSPIMemInfo->hMemParams->memType = SPI_MEM_TYPE_FLASH;
    
      // Send dummy data, receive device ID1
      spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);

      // Send dummy data, receive device ID1
      spibuf = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);
      
      // FIXME: Add detection of page, sector, block sizes
    }
    else
    {
      hSPIMemInfo->hMemParams->memType = SPI_MEM_TYPE_EEPROM;
    }

    SPI_disableCS(hSPIMemInfo->hSPIInfo);
  }

  return hSPIMemInfo;
}

// Routine to read data from SPI
Uint32 SPI_MEM_readBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *dest)
{
  Uint32 status;

  SPI_enableCS(hSPIMemInfo->hSPIInfo);
  
  // Send command and address
  LOCAL_xferCmdAddrBytes(hSPIMemInfo, SPI_MEM_CMD_READ, addr);
  
  // Receive data bytes
  status = SPI_xferBytes(hSPIMemInfo->hSPIInfo, byteCnt, dest);
  
  SPI_disableCS(hSPIMemInfo->hSPIInfo);

  return status;
}


// Defining this macro for the build will cause write (flash) ability to be removed
// This can be used for using this driver as read-only for ROM code
#ifndef USE_IN_ROM

// Generic routine to write data to SPI
Uint32 SPI_MEM_writeBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *src)
{
  Uint32  status;
  
  while (byteCnt >= hSPIMemInfo->hMemParams->pageSize)
  {
    // Issue WREN command
    LOCAL_issueWRENCommand(hSPIMemInfo);  
  
    SPI_enableCS(hSPIMemInfo->hSPIInfo);

    // Send command and address
    LOCAL_xferCmdAddrBytes(hSPIMemInfo, SPI_MEM_CMD_WRITE, addr);

    // Send bytes to write
    status = SPI_xferBytes(hSPIMemInfo->hSPIInfo, hSPIMemInfo->hMemParams->pageSize, src);

    SPI_disableCS(hSPIMemInfo->hSPIInfo);
    
    if (status != E_PASS) return status;

    // Wait until write is complete
    LOCAL_waitForReady(hSPIMemInfo);
    
    byteCnt -= hSPIMemInfo->hMemParams->pageSize;
    src     += hSPIMemInfo->hMemParams->pageSize;
    addr    += hSPIMemInfo->hMemParams->pageSize;
  }
  
  // Issue WREN command
  LOCAL_issueWRENCommand(hSPIMemInfo);
  
  SPI_enableCS(hSPIMemInfo->hSPIInfo);

  // Send command and address
  LOCAL_xferCmdAddrBytes(hSPIMemInfo, SPI_MEM_CMD_WRITE, addr);

  // Send bytes to write
  status = SPI_xferBytes(hSPIMemInfo->hSPIInfo, byteCnt, src);

  SPI_disableCS(hSPIMemInfo->hSPIInfo);
  
  if (status != E_PASS) return status;  

  // Wait until write is complete
  LOCAL_waitForReady(hSPIMemInfo);

  return status;
}

// Verify data written by reading and comparing byte for byte
Uint32 SPI_MEM_verifyBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *src, Uint8* dest)
{
  Uint32 i;

  if (SPI_MEM_readBytes(hSPIMemInfo,addr,byteCnt,dest) != E_PASS)
    return E_FAIL;

  for (i=0; i<byteCnt; i++)
  {
    if (dest[i] != src[i]) return E_FAIL;
  }

  return E_PASS;
}

// Global SPI memory erase
Uint32 SPI_MEM_globalErase(SPI_MEM_InfoHandle hSPIMemInfo)
{
  if (hSPIMemInfo->hMemParams->memType == SPI_MEM_TYPE_EEPROM)
  {
    SPI_MEM_eraseBytes(hSPIMemInfo,0x0,hSPIMemInfo->hMemParams->memorySize);
  }
  else if (hSPIMemInfo->hMemParams->memType == SPI_MEM_TYPE_FLASH)
  {
    LOCAL_SPIFlash_bulkErase(hSPIMemInfo);
  }
  else
  {
    return E_FAIL;
  } 

  return E_PASS;
}

// SPI memory erase
Uint32 SPI_MEM_eraseBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 startAddr, Uint32 byteCnt)
{
  Uint8 buffer[SPI_MAX_PAGE_SIZE];
  Uint32 eepromAddr = startAddr;
  Uint32 bytesLeft = byteCnt; 
  Uint32 i;

  if (hSPIMemInfo->hMemParams->memType == SPI_MEM_TYPE_EEPROM)
  {
    // Create erase buffer
    for (i=0; i<hSPIMemInfo->hMemParams->pageSize; i+=4)
    {
      *((Uint32 *)(buffer+i)) = 0xFFFFFFFF;
    }
    
    while(bytesLeft >= hSPIMemInfo->hMemParams->pageSize)
    {
      SPI_MEM_writeBytes(hSPIMemInfo,eepromAddr,hSPIMemInfo->hMemParams->pageSize, buffer);
      bytesLeft -= hSPIMemInfo->hMemParams->pageSize;
      eepromAddr += hSPIMemInfo->hMemParams->pageSize;
    }
    
    SPI_MEM_writeBytes(hSPIMemInfo, eepromAddr, bytesLeft, buffer);
    
    return E_PASS;
  }
  else if (hSPIMemInfo->hMemParams->memType == SPI_MEM_TYPE_FLASH)
  {
    // Do bulk (chip) erase if appropriate
    if ( hSPIMemInfo->hMemParams->memorySize != 0 )
    {
      if ( ( bytesLeft >= hSPIMemInfo->hMemParams->memorySize ) ||
           ( (hSPIMemInfo->hMemParams->blockSize == 0) && (hSPIMemInfo->hMemParams->sectorSize == 0) ) )
      {
        // Do chip erase
        LOCAL_SPIFlash_bulkErase(hSPIMemInfo);
        
      }
    }
    
    // Do block erase if appropriate
    if (hSPIMemInfo->hMemParams->blockSize != 0)
    {
      if ( (bytesLeft >= hSPIMemInfo->hMemParams->blockSize) || (hSPIMemInfo->hMemParams->sectorSize == 0) )
      {
        // Do block erase
        Uint32 mask = ~(hSPIMemInfo->hMemParams->blockSize - 1);
        Uint32 endBlockAddr = (startAddr + bytesLeft) & mask;
        Uint32 currBlockAddr = startAddr & mask;
        
        while (currBlockAddr <= endBlockAddr)
        {
          LOCAL_SPIFlash_blockErase(hSPIMemInfo, eepromAddr);
          bytesLeft -= hSPIMemInfo->hMemParams->blockSize;
          eepromAddr += hSPIMemInfo->hMemParams->blockSize;
          currBlockAddr = eepromAddr & mask;
        }
        return E_PASS;
      }
    }
    
    // Do sector erase if appropriate
    if (hSPIMemInfo->hMemParams->sectorSize != 0)
    {
      // Do sector erase
      Uint32 mask = ~(hSPIMemInfo->hMemParams->sectorSize - 1);
      Uint32 endSectAddr = (startAddr + bytesLeft) & mask;
      Uint32 currSectAddr = startAddr & mask;
      
      while (currSectAddr <= endSectAddr)
      {
        LOCAL_SPIFlash_sectorErase(hSPIMemInfo, eepromAddr);
        bytesLeft -= hSPIMemInfo->hMemParams->sectorSize;
        eepromAddr += hSPIMemInfo->hMemParams->sectorSize;
        currSectAddr = eepromAddr & mask;
      }
      return E_PASS;
    }
  }
    
  return E_FAIL;
}
#endif

/************************************************************
* Local Function Definitions                                *
************************************************************/

static Uint32 LOCAL_xferCmdAddrBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint8 command, Uint32 addr)
{
  Uint8  cmdAddrBuff[4];
  Uint32 i;
  
  // Send memory write command
  cmdAddrBuff[0] = command;

  for (i=0; i<(hSPIMemInfo->hMemParams->addrWidth>>3); i++)
  {
    cmdAddrBuff[i+1] = ((addr >> (hSPIMemInfo->hMemParams->addrWidth - ((i + 1)*8))) & 0xFF);
  }
  
  return SPI_xferBytes(hSPIMemInfo->hSPIInfo, 1 + (hSPIMemInfo->hMemParams->addrWidth>>3), cmdAddrBuff);  
}

static Uint32 LOCAL_waitForReady(SPI_MEM_InfoHandle hSPIMemInfo)
{
  Uint8 statusReg;

  // Poll Status to make sure it is ready
  do
  {
    // Send Read Status Register Commeand
    SPI_enableCS(hSPIMemInfo->hSPIInfo);
    SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_RDSR);
 
    statusReg = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);    
    SPI_disableCS(hSPIMemInfo->hSPIInfo);
  }
  while((statusReg & 0x1u) == 0x1);
  
  return E_PASS;
}

// Defining this macro for the build will cause write (flash) ability to be removed
// This can be used for using this driver as read-only for ROM code
#ifndef USE_IN_ROM

static void LOCAL_issueWRENCommand(SPI_MEM_InfoHandle hSPIMemInfo)
{
  Uint8 statusReg;
  
  // Issue write enable command
  SPI_enableCS(hSPIMemInfo->hSPIInfo);
  SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_WREN);
  SPI_disableCS(hSPIMemInfo->hSPIInfo);
  
  // Poll EEPROM Status to make sure Write Enable Latch has been set
  do
  {
    // Send Read Status Register Commeand  
    SPI_enableCS(hSPIMemInfo->hSPIInfo);
    SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_RDSR);
  
    statusReg = SPI_xferOneChar(hSPIMemInfo->hSPIInfo,0x00);
    SPI_disableCS(hSPIMemInfo->hSPIInfo);
  }
  while((statusReg & 0x3u) != 0x2);
}

static void LOCAL_SPIFlash_bulkErase(SPI_MEM_InfoHandle hSPIMemInfo)
{  
  // Issue WREN command
  LOCAL_issueWRENCommand(hSPIMemInfo);

  // Issue Bulk Erase Command
  SPI_enableCS(hSPIMemInfo->hSPIInfo);
  SPI_xferOneChar(hSPIMemInfo->hSPIInfo,SPI_MEM_CMD_CHIPERASE);
  SPI_disableCS(hSPIMemInfo->hSPIInfo);
  
  // Poll EEPROM Status to make sure it is ready
  LOCAL_waitForReady(hSPIMemInfo); 
}

static void LOCAL_SPIFlash_blockErase(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 blockAddr)
{
  // Issue WREN command
  LOCAL_issueWRENCommand(hSPIMemInfo);
  
  // Send block erase command to memory
  SPI_enableCS(hSPIMemInfo->hSPIInfo);
  LOCAL_xferCmdAddrBytes(hSPIMemInfo, SPI_MEM_CMD_BLOCKERASE, blockAddr);
  SPI_disableCS(hSPIMemInfo->hSPIInfo);
  
  // Poll EEPROM Status to make sure it is ready
  LOCAL_waitForReady(hSPIMemInfo);
}

static void LOCAL_SPIFlash_sectorErase(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 sectorAddr)
{
  // Issue WREN command
  LOCAL_issueWRENCommand(hSPIMemInfo);
  
  // Send block erase command to memory
  SPI_enableCS(hSPIMemInfo->hSPIInfo);
  LOCAL_xferCmdAddrBytes(hSPIMemInfo, SPI_MEM_CMD_SECTORERASE, sectorAddr);
  SPI_disableCS(hSPIMemInfo->hSPIInfo);
  
  // Poll EEPROM Status to make sure it is ready
  LOCAL_waitForReady(hSPIMemInfo);  
}

#endif


/***********************************************************
* End file                                                 *
***********************************************************/

