/* --------------------------------------------------------------------------
    FILE        : device_spi.c 				                             	 	        
    PROJECT     : TI Booting and Flashing Utilities
    AUTHOR      : Daniel Allred
    DESC        : This file descibes and implements various device-specific NAND
                  functionality.
-------------------------------------------------------------------------- */ 

// General type include
#include "tistdtypes.h"

// Device specific CSL
#include "device.h"

// Device specific SPI details
#include "device_spi.h"

// Generic SPI header file
#include "spi.h"
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


/************************************************************
* Local Variable Definitions                                *
************************************************************/


/************************************************************
* Global Variable Definitions                               *
************************************************************/

const Uint32 DEVICE_SPI_baseAddr[SPI_PERIPHERAL_CNT] =
{
  (Uint32) SPI0,
  (Uint32) SPI1,
  (Uint32) SPI2,
  (Uint32) SPI3,
  (Uint32) SPI4
};

const SPI_ConfigObj DEVICE_SPI_config = 
{
  1,        // polarity
  0,        // phase
  79,       // prescalar
  8         // charLen
};

// Set SPI config to NULL to use SPI driver defaults
//SPI_ConfigHandle const hDEVICE_SPI_config = NULL;
SPI_ConfigHandle const hDEVICE_SPI_config = (SPI_ConfigHandle) &DEVICE_SPI_config;

const SPI_MEM_ParamsObj DEVICE_SPI_MEM_params = 
{
  SPI_MEM_TYPE_FLASH,
  24,                     // addrWidth
  256,                    // pageSize
  4096,                   // sectorSize
  64*1024,                // blockSize
  512*1024                // memorySize           
};

// Set mem params to NULL to enforce autodetect
//SPI_MEM_ParamsHandle const hDEVICE_SPI_MEM_params = NULL;
SPI_MEM_ParamsHandle const hDEVICE_SPI_MEM_params = (SPI_MEM_ParamsHandle) &DEVICE_SPI_MEM_params;


/************************************************************
* Global Function Definitions                               *
************************************************************/


/************************************************************
* Local Function Definitions                                *
************************************************************/


/************************************************************
* End file                                                  *
************************************************************/


