/* --------------------------------------------------------------------------
  FILE        : spi_mem.h
  PROJECT     : DaVinci User Boot-Loader and Flasher
  AUTHOR      : Texas Instruments Incorporated
  DESC        : Generic SPI Memory driver header file
 ----------------------------------------------------------------------------- */
 
#ifndef _SPI_MEM_H_
#define _SPI_MEM_H_

#include "tistdtypes.h"
#include "spi.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

/************************************************************
* Global Macro Declarations                                 *
************************************************************/

#define SPI_MAX_PAGE_SIZE   (64)


/***********************************************************
* Global Typedef declarations                              *
***********************************************************/

typedef enum _SPI_MEM_TYPE_
{
  SPI_MEM_TYPE_EEPROM       = 0x01,
  SPI_MEM_TYPE_FLASH        = 0x02,
  SPI_MEM_TYPE_ATMELFLASH   = 0x04
}
SPI_MEM_Type;

// SPI Memory device commands (Master mode)
typedef enum _SPI_MEM_COMMANDS_
{
  SPI_MEM_CMD_WREN = 0x06,
  SPI_MEM_CMD_WRDI = 0x04,
  SPI_MEM_CMD_RDSR = 0x05,
  SPI_MEM_CMD_WRSR = 0x01,
  SPI_MEM_CMD_READ = 0x03,
  SPI_MEM_CMD_WRITE = 0x02,
  SPI_MEM_CMD_FASTREAD = 0x0B,                // SPI Flash only 
  SPI_MEM_CMD_FASTREAD_DUAL_OUTPUT = 0x3B,    // SPI Flash only 
  SPI_MEM_CMD_BLOCKERASE = 0xD8,              // SPI Flash only 
  SPI_MEM_CMD_SECTORERASE = 0x20,             // SPI Flash only 
  SPI_MEM_CMD_CHIPERASE = 0xC7,               // SPI Flash only 
  SPI_MEM_CMD_POWERDOWN = 0xB9,               // SPI Flash only 
  SPI_MEM_CMD_RELEASE_POWERDOWN = 0xAB,       // SPI Flash only 
  SPI_MEM_CMD_SIMPLE_ID = 0x90,               // SPI Flash only 
  SPI_MEM_CMD_JEDEC_ID = 0x9F                 // SPI Flash only 
}
SPI_MEM_Commands;

typedef struct _SPI_MEM_CMDS_
{
  Uint8 readCmd;
  Uint8 writeCmd;
}
SPI_MEM_Cmds;

typedef struct _SPI_MEM_PARAMS_
{
  SPI_MEM_Type memType;
  Uint16 addrWidth;
  Uint16 pageSize;
  Uint32 sectorSize;
  Uint32 blockSize;
  Uint32 memorySize;
}
SPI_MEM_ParamsObj, *SPI_MEM_ParamsHandle;

// SPI driver structure
typedef struct _SPI_MEM_INFO_
{
  SPI_InfoHandle hSPIInfo;
  SPI_MEM_ParamsHandle hMemParams; 
}
SPI_MEM_InfoObj, *SPI_MEM_InfoHandle;


/************************************************************
* Global Function Declarations                              *
************************************************************/

extern SPI_MEM_InfoHandle SPI_MEM_open(Uint32 spiPeripheralNum);
extern Uint32 SPI_MEM_readBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *dest);
extern Uint32 SPI_MEM_writeBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *src);
extern Uint32 SPI_MEM_verifyBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 addr, Uint32 byteCnt, Uint8 *src, Uint8* dest);
extern Uint32 SPI_MEM_globalErase(SPI_MEM_InfoHandle hSPIMemInfo);
extern Uint32 SPI_MEM_eraseBytes(SPI_MEM_InfoHandle hSPIMemInfo, Uint32 startAddr, Uint32 byteCnt);



/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif

#endif //_SPI_H_

