/* --------------------------------------------------------------------------
  FILE        : spi.h
  PROJECT     : DaVinci User Boot-Loader and Flasher
  AUTHOR      : Texas Instruments Incorporated
  DESC        : Generic SPI driver header file
 ----------------------------------------------------------------------------- */
 
#ifndef _SPI_H_
#define _SPI_H_

#include "tistdtypes.h"

// Prevent C++ name mangling
#ifdef __cplusplus
extern far "c" {
#endif

/************************************************************
* Global Macro Declarations                                 *
************************************************************/

#define SPI_SPIFLG_TXINTFLAG     (0x00000200u)
#define SPI_SPIFLG_RXINTFLAG     (0x00000100u)
#define SPI_SPIFLG_OVRNINTFLG    (0x00000040u)
#define SPI_SPIFLG_BITERRFLG     (0x00000010u)
#define SPI_SPIFLG_DESELECTFLG   (0x00000004u)            // SPI_PARERRFLG


/***********************************************************
* Global Typedef declarations                              *
***********************************************************/

// SPI Module Roles
typedef enum _SPI_ROLE_
{
  SPI_ROLE_MASTER = 0x01,
  SPI_ROLE_SLAVE = 0x02
}
SPI_Role;

// SPI Module Modes of Operation
typedef enum _SPI_MODE_
{
  SPI_MODE_3PIN       = 0x01,
  SPI_MODE_4PIN_CS    = 0x02,
  SPI_MODE_4PIN_ENA   = 0x04,
  SPI_MODE_5PIN       = 0x08
}
SPI_Mode;

typedef struct _SPI_CONFIG_
{
  Uint8 polarity;
  Uint8 phase;
  Uint8 prescalar;
  Uint8 charLen;
}
SPI_ConfigObj, *SPI_ConfigHandle;

// SPI driver structure
typedef struct _SPI_INFO_
{
  Uint32            peripheralNum;
  void              *regs;
  SPI_Role          role;
  SPI_Mode          mode;
  SPI_ConfigHandle  config;
}
SPI_InfoObj, *SPI_InfoHandle;

/***********************************************************
* Global Variable declarations                             *
***********************************************************/

extern __FAR__ SPI_ConfigHandle const hDEFAULT_SPI_CONFIG;


/************************************************************
* Global Function Declarations                              *
************************************************************/

extern __FAR__ SPI_InfoHandle SPI_open(Uint32 spiPeripheralNum, SPI_Role role, SPI_Mode mode, SPI_ConfigHandle config);
extern __FAR__ Uint32 SPI_reset(SPI_InfoHandle hSPIInfo);
extern __FAR__ Uint32 SPI_readBytes(SPI_InfoHandle hSPIInfo, Uint32 byteCnt, Uint8 *dest);
extern __FAR__ Uint32 SPI_writeBytes(SPI_InfoHandle hSPIInfo, Uint32 byteCnt, Uint8 *src);
extern __FAR__ Uint32 SPI_xferBytes(SPI_InfoHandle hSPIInfo, Uint32 byteCnt, Uint8 *src);
extern __FAR__ Uint8  SPI_xferOneChar(SPI_InfoHandle hSPIInfo, Uint8 dataOut);
extern __FAR__ void   SPI_enableCS(SPI_InfoHandle hSPIInfo);
extern __FAR__ void   SPI_disableCS(SPI_InfoHandle hSPIInfo);


/***********************************************************
* End file                                                 *
***********************************************************/

#ifdef __cplusplus
}
#endif

#endif //_SPI_H_

