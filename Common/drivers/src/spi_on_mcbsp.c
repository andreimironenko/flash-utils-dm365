/*  ============================================================================
 *   Copyright (c) Texas Instruments Inc 2002, 2003, 2004
 *
 *   Use of this software is controlled by the terms and conditions found in the
 *   license agreement under which this software has been supplied.
 *   ===========================================================================
 */

/** @file spi.c
 *
 *  @brief File contains code to Program and Verify Write of Data to SPI EEPROM
 *         via MCBSP0 of device
 *
 *  Modification 1
 *    - Created on: May 24, 2007
 *
 *  @author anonymous
 */
 
#include "ubl.h"
#include "target.h"
#include "stdio.h"
#include "spi.h"

                
/**************************************************************************\
* SPI Global Declarations
\**************************************************************************/	
#define DRIVE_CS_LOW {GPIO6->CLR_DATA6 = GPIO97_MASK;}
#define DRIVE_CS_HIGH {GPIO6->SET_DATA6 = GPIO97_MASK;}

volatile near SPI_INFO spiInfo;
Uint8 ch;

	 
//------------------------------------------------------------------------
//------------------------------------------------------------------------

Uint32 SPI_WriteHeaderAndData(Uint8 *ramPtr)
{	 
  Uint32 j;
  Uint32 errCnt;
  Uint32 eepromAddr;
  Uint32 rcv;
  
  printf("Erasing Previous Data From EEPROM\n");
  printf(" ... please be patient as this takes a while, thanks.\n");
  printf(" ... Erasing %d bytes.\n ",spiInfo.fileSize);

  eepromAddr = 0;
  j = 0;
  ch = 0xFF;
  while(j<(spiInfo.fileSize))
  {
    SPI_WriteByte(eepromAddr, &ch);
	  eepromAddr++;
	  j++;
  }

  printf("Verifying erase of data\n");
  eepromAddr = 0;
  j = 0;
  errCnt = 0;
  
  while(j<(spiInfo.fileSize))
  {
    rcv = (SPI_ReadByte(eepromAddr) & 0xFFu);
	  if (rcv != 0xFF)
    {
		  errCnt++;
	  }
	  eepromAddr++;
	  j++;
  }
  
  printf("Error count = %d\n",errCnt);
  
  if (errCnt) 
  {
    printf("Erase failed! Aborting...");
    return E_FAIL;
  }


  eepromAddr = 0x0000;
  j = 0;

  printf("Writing Data to SPI EEPROM\n");
  printf(" ... please be patient this takes a while, thanks\n");
  printf(" ... File Size in Bytes = %d\n",spiInfo.fileSize);
  
  while(j<(spiInfo.fileSize))
  {
    SPI_WriteByte(eepromAddr, &ramPtr[j]);
	  eepromAddr++;
	  j++;
  }
  
  // Allow some time to make sure all writes complete     
  waitloop(1000);
  
  eepromAddr = 0x0000; 
  j = 0;
  errCnt = 0;

  printf("Data Write Complete .. verifying data \n");
  while(j<spiInfo.fileSize)
  {
    rcv = (SPI_ReadByte(eepromAddr) & 0xFFu);
	  if (rcv != ramPtr[j]) 
    {
		  errCnt++;
      printf("Index=%d, Read byte=%x, Expected byte=%x.\n",j,rcv,ramPtr[j]);
	  }
	  eepromAddr++;
	  j++;
  }

  if (errCnt) 
  {
    printf("Data verify failed! Aborting...");
    return E_FAIL;
  }
      
  return E_PASS;
}

 

Uint32  SPI_Init(void)
{
  Uint32 tmp, spcr, srgr, mcr, xcr, rcr, pcr;

  /* Configure MCBSP0 PINS as MCBSP Pins */
  tmp = SYSTEM->PINMUX[1] & (~SYS_PINMUX1_MB0BK_MASK);
  tmp |= (1u << SYS_PINMUX1_MB0BK_SHIFT);
  SYSTEM->PINMUX[1] = tmp;

  // Setup Pin Mux for GPIO - we are using GPIO97 (TOUTL0) for CS
  tmp = SYSTEM->PINMUX[1] & (~SYS_PINMUX1_TIM0BK_MASK);
  SYSTEM->PINMUX[1] = tmp;


  /* Make sure MCBSP0 is powered up */
  SYSTEM->VDD3P3V_PWDN &= ~(SYS_VDD3P3V_PWRDN_MCBSP_MASK);
  SYSTEM->VDD3P3V_PWDN &= ~(SYS_VDD3P3V_PWRDN_TMR0_MASK);

  //----------------------------------------------------------------------
  // Configure GPIO PIN GIO97 as Output Pin
  //----------------------------------------------------------------------
   LPSCTransition(LPSC_GPIO, PSC_ENABLE);
  tmp = GPIO6->DIR6;
  tmp = tmp & ~(GPIO97_MASK); 
  GPIO6->DIR6 = tmp;

  //----------------------------------------------------------------------
  // Drive CS High
  //----------------------------------------------------------------------	 
  DRIVE_CS_HIGH;
   while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){
  }

  // Make Sure MCBSP0 is powered up and ready
  LPSCTransition(LPSC_MCBSP0, PSC_ENABLE);    

	             
  /* Set SPCR to defaults, Hold Sample-Rate Generator,Transmit/Receive
  in reset                                                           */
  spcr = 0x00000000u; //MCBSP0->SPCR = 0x00000000u;
  mcr  = 0x00000000u;  //MCBSP0->MCR  = 0x00000000u;
  xcr  = 0x00000000u;
  rcr  = 0x00000000u;
  srgr = 0x00000000u;
  pcr  = 0x00000000u;
	                      

  //----------------------------------------------------------------------
  // Set McBSP in SPI Master mode, normally FSX0 would be used
  // as chip-select but to support SPI devices with 24BIT address
  // widht - FSX0 although generated, will be ignored. The SPI, CS, will
  // be connected to TOUT0L, configured as GPIO pin.
  //----------------------------------------------------------------------   
  pcr = pcr |   (  MCBSP_PCR_FSXM_MASK | 
	                 MCBSP_PCR_CLKXM_MASK |
	                 MCBSP_PCR_FSXP_MASK |
	                 MCBSP_PCR_FSRP_MASK );
	          
	               
  //------------------------------------------------------------------------
  // For SPI mode a XMIT and RCV delya of 1 is required. The XMIT and RCV
  // frames have been set to send/receive 8 bits.
  //------------------------------------------------------------------------
  rcr = rcr  | (1u << MCBSP_RCR_RDATDLY_SHIFT)
              | (0u << MCBSP_RCR_RWDLEN1_SHIFT) 
              | (0u << MCBSP_RCR_RFRLEN1_SHIFT)	                                              
              | (0u << MCBSP_RCR_RWDLEN2_SHIFT);

  xcr = xcr  | (1u << MCBSP_XCR_XDATDLY_SHIFT)
              | (0u << MCBSP_XCR_XWDLEN1_SHIFT) 
              | (0u << MCBSP_XCR_XFRLEN1_SHIFT)	                                              
              | (0u << MCBSP_XCR_XWDLEN2_SHIFT);	

   // Should give 1MHZ with ByPass Mode 27MHZ clock
  srgr = srgr | MCBSP_SRGR_CLKSM_MASK
              | (0x40u << MCBSP_SRGR_FPER_SHIFT)
              | (0x1u << MCBSP_SRGR_FWID_SHIFT) 
              | (0x25u  << MCBSP_SRGR_CLKGDV_SHIFT);
	      
	                      
  /* Set polarity and phase of CLK using CLKSTP and CLKXP */
  spcr = (spcr &( ~MCBSP_SPCR_CLKSTP_MASK));
	                        
	 MCBSP0->MCR = mcr;
	 MCBSP0->PCR = pcr;
	 MCBSP0->XCR = xcr;
	 MCBSP0->RCR = rcr;
	 MCBSP0->SRGR = srgr;
	 MCBSP0->SPCR = spcr;




	/*************************************************************************/
	/* \brief  Configure the MCBSP with following values.
	*************************************************************************
	** \note McBSP receiver, transmitter frame sync logic and sample rate
 	** generator are not enabled in this function. This is done because
 	** once these are enabled, McBSP will start its operation, which might
 	** not be required till DMA and IRQ setups, if any are done.
 	*************************************************************************/

	//Wait two CLKSRG clocks. This is to ensure proper synchronization internally.
  	asm("\tnop 5");

	MCBSP0->SPCR = (MCBSP0->SPCR & (~MCBSP_SPCR_CLKSTP_MASK))
	                 |(0x3u << MCBSP_SPCR_CLKSTP_SHIFT);

 	/* Initialization of transmit values and array of received values */

	/* SPI-EEPROM data packet represented in 32 bit format
 	 * first byte is write enable opcode
     * second, third bytes represnt 16 bit address,
     * fourth byte is the actual data to be written 
     */
   /* Start Sample Rate Generator Clock Generation */
  MCBSP0->SPCR |= MCBSP_SPCR_FREE_MASK;
  waitloop(20);

  MCBSP0->SPCR |= MCBSP_SPCR_GRST_MASK;// grstControl (enable);
  waitloop(20);
  /* Start Sample Rate Generator Frame Generation */

  MCBSP0->SPCR |= MCBSP_SPCR_FRST_MASK; //MCBSP_frstControl (enable);
  waitloop(20);

  /* Enable Receiver */
  MCBSP0->SPCR |= MCBSP_SPCR_RRST_MASK; //rrstControl  (enable);
  waitloop(20);
  /* Enable Transmitter */

  MCBSP0->SPCR |= MCBSP_SPCR_XRST_MASK; //xrstControl (enable);
  waitloop(20);

  //Wait two CLKSRG clocks. This is to ensure proper synchronization internally.
  asm("\tnop 5");

  return E_PASS;
}

Uint32  SPI_ReadByte(Uint32 eeprom_addr)
{
  Uint32 byte32,i;
  Uint32 read_cmd = 0x03000000u;

	//--------------------------------------------------------------
  DRIVE_CS_LOW;
  while((GPIO6->SET_DATA6 & GPIO97_MASK)){	 }

  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)(((read_cmd | eeprom_addr)>>24)&0xFF);

  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)((eeprom_addr>>8)&0xFF);

  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)(eeprom_addr & 0xFF);

  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = 0;

	while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
	for(i=0;i<0x10;++i){	}

  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  DRIVE_CS_HIGH;  
  while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){  }
	for(i=0;i<0x60;++i){	 }

  return (byte32 & 0xFF);
}
 

 
Uint32 SPI_WriteByte(Uint32 eeprom_addr, Uint8 *byte)
{
  Uint32 byte32;
  Uint32 statusReg;
  Uint32 spi_cmd = SPI_PROG_COMMAND;
 
  
  // Send Write Enable Command to EEPROM
  DRIVE_CS_LOW;
  while((GPIO6->SET_DATA6 & GPIO97_MASK)){  }   
  waitloop(0x20);

  // Poll for XRDY flag to acknowledge transmitter  is ready */	 
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)((SPI_WREN_COMMAND >>24u)&0xFF) ;   
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    
  // Poll for RRDY flag to acknowledge receiver is ready */
  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  waitloop(0x20);
  DRIVE_CS_HIGH;
  while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){		 }                     
  // Needed some delay 
  waitloop(1000);


  
  // Poll EEPROM Status to make sure Write Enable Latch has been set
  do
  {
    DRIVE_CS_LOW;
    while((GPIO6->SET_DATA6 & GPIO97_MASK)){		 }
		waitloop(0x20);

    // Send  WRSR opcode to read status register */
    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = (Uint32)((SPI_RSR_COMMAND >> 24u) & 0xFF);
    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (Uint32)(MCBSP0->DRR);

    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = 0;
    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (Uint32)(MCBSP0->DRR);

    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = 0;
    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (statusReg << 8 ) | (Uint32)(MCBSP0->DRR);

		waitloop(0x20);
		  
    DRIVE_CS_HIGH;
    while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){  }
  }
  while((statusReg & 0xFu) != 0x2);	       
	waitloop(60);

  
  
  // Write Data Byte to EEPROM	    
  DRIVE_CS_LOW;
  while((GPIO6->SET_DATA6 & GPIO97_MASK)){  }
  waitloop(0x20);

  // Program command
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)(((spi_cmd | eeprom_addr)>>24)&0xFF);
  //	while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  // Address byte 1
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)((eeprom_addr>>8)&0xFF);
  //	while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  // Address byte 2
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = (Uint32)(eeprom_addr & 0xFF);
  //	while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  // Data to program
  while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  MCBSP0->DXR = *byte;
  //	while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
  while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
  byte32 = (Uint32)(MCBSP0->DRR);

  waitloop(0x20);
  DRIVE_CS_HIGH;
  while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){  }



  // Poll EEPROM Status to see if Write in Progress (WIP) bit is set)
  do
  {
    DRIVE_CS_LOW;
    while((GPIO6->SET_DATA6 & GPIO97_MASK)){  }
    waitloop(0x20);

    // Send  WRSR opcode to read status register */
    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = (Uint32)((SPI_RSR_COMMAND >> 24u) & 0xFF);

    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (Uint32)(MCBSP0->DRR);

    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = 0;

    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (Uint32)(MCBSP0->DRR);

    while ((MCBSP0->SPCR & MCBSP_SPCR_XRDY_MASK) != MCBSP_SPCR_XRDY_MASK);
    MCBSP0->DXR = 0;

    while ((MCBSP0->SPCR & MCBSP_SPCR_RRDY_MASK) != MCBSP_SPCR_RRDY_MASK);
    statusReg = (statusReg << 8 ) | (Uint32)(MCBSP0->DRR);

    waitloop(0x20);
    DRIVE_CS_HIGH;
    while(!(GPIO6->SET_DATA6 & GPIO97_MASK)){  }
  }
  while((statusReg & 0x1u) != 0x1);
	 	      
  waitloop(1000);
  
  return E_PASS;
} 

 //---------------------------------------------------------------------------
 // Revision History
 //    May 24, 2007 - Created
 //---------------------------------------------------------------------------
