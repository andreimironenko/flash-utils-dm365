/* --------------------------------------------------------------------------
  FILE        : device.c 				                             	 	        
  PURPOSE     : Device (chip) specific initialization function
  PROJECT     : DA8xx ROM boot loader
  AUTHOR      : Daniel Allred
  DESC        : This file provides low-level init functions for use
                  in the UBL for booting an application.
-------------------------------------------------------------------------- */ 

// General type include
#include "tistdtypes.h"

// This module's header file
#include "device.h"

// Debug module
#include "debug.h"

// Utility functions
#include "util.h"


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


/************************************************************
* Local Variable Definitions                                *
\***********************************************************/


/************************************************************
* Global Variable Definitions                               *
************************************************************/

const String devString = "OMAP-L137";

#define SKIP_LOW_LEVEL_INIT /*skip the low-level define PLL and DDR2*/

/************************************************************
* Global Function Definitions                               *
************************************************************/
Uint32 DEVICE_init()
{
  Uint32 status = E_PASS;

#ifndef SKIP_LOW_LEVEL_INIT
  // System PSC Setup
  DEVICE_PSCInit();

  // System PLL Setup
  if (status == E_PASS) status |= DEVICE_PLL1Init(DEVICE_PLL1_MULTIPLIER);
  
  // DDR PLL Setup
  if (status == E_PASS) status |= DEVICE_PLL2Init(DEVICE_PLL1_MULTIPLIER);
  
  // DDR2 Timing Setup
  if (status == E_PASS) status |= DEVICE_DDR2Init();
#endif

  // AEMIF Setup
  if (status == E_PASS) status |= DEVICE_EMIFInit();

  return status;
}


void DEVICE_LPSCTransition(Uint8 pscnum, Uint8 module, Uint8 domain, Uint8 state)
{
  DEVICE_PSCRegs *PSC;
  
  if (pscnum == 0)
    PSC = PSC0;
  else if(pscnum == 1)
    PSC = PSC1;
  else
    return; 

  // Wait for any outstanding transition to complete
  while ( (PSC->PTSTAT) & (0x00000001 << domain) );
  
  // If we are already in that state, just return
  if (((PSC->MDSTAT[module]) & 0x1F) == state) return;
    
  // Perform transition
  PSC->MDCTL[module] = ((PSC->MDCTL[module]) & (0xFFFFFFE0)) | (state);
  PSC->PTCMD |= (0x00000001 << domain);

  // Wait for transition to complete
  while ( (PSC->PTSTAT) & (0x00000001 << domain) );
  
  // Wait and verify the state
  while (((PSC->MDSTAT[module]) & 0x1F) != state);	
}

void DEVICE_PSCInit()
{                                                                    
  Uint32 i;
  
  // PSC0, domain 0 init
  while ((PSC0->PTSTAT) & 0x00000001);

  for( i = 0 ; i <= 6 ; i++ )
    PSC0->MDCTL[i] = (PSC0->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;

  for( i = 8 ; i <= 13 ; i++ )
    PSC0->MDCTL[i] = (PSC0->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;
    
  // Do Always-On Power Domain Transitions
  PSC0->PTCMD |= 0x00000001;
  while ((PSC0->PTSTAT) & 0x00000001);
  
  // PSC1, domain 1 init
  while ((PSC1->PTSTAT) & 0x00000001);

  for( i = 1 ; i <= 13 ; i++ )
    PSC1->MDCTL[i] = (PSC1->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;

  for( i = 16 ; i <= 17 ; i++ )
    PSC1->MDCTL[i] = (PSC1->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;

  for( i = 20 ; i <= 21 ; i++ )
    PSC1->MDCTL[i] = (PSC1->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;
  
  for( i = 24 ; i <= 26 ; i++ )
    PSC1->MDCTL[i] = (PSC1->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;

  for( i = 31 ; i <= 31 ; i++ )
    PSC1->MDCTL[i] = (PSC1->MDCTL[i] & 0xFFFFFFE0) | PSC_ENABLE;
    
  // Do Always-On Power Domain Transitions
  PSC1->PTCMD |= 0x00000001;
  while ((PSC1->PTSTAT) & 0x00000001);

}

void DEVICE_pinmuxControl(Uint32 regOffset, Uint32 mask, Uint32 value)
{
  SYSTEM->KICKR[0] = 0x83e70b13;  // Kick0 register + data (unlock)
  SYSTEM->KICKR[1] = 0x95a4f1e0;  // Kick1 register + data (unlock)

  SYSTEM->PINMUX[regOffset] &= ~mask;
  SYSTEM->PINMUX[regOffset] |= (mask & value);

  SYSTEM->KICKR[0] = 0x00000000;  // Kick0 register + data (lock)
  SYSTEM->KICKR[1] = 0x00000000;  // Kick1 register + data (lock)
}

DEVICE_BootMode DEVICE_bootMode( void )
{
  // FIXME to actually detect the correct bootmode
  return DEVICE_BOOTMODE_NAND_EMIFA_8BIT;
}

DEVICE_BusWidth DEVICE_emifBusWidth( void )
{
  if (DEVICE_bootMode() == DEVICE_BOOTMODE_NAND_EMIFA_8BIT)
  {
    return DEVICE_BUSWIDTH_8BIT;
  }
  else
  {
    return DEVICE_BUSWIDTH_16BIT;
  }
}

Uint32 DEVICE_EMIFInit()
{

  DEVICE_pinmuxControl(13,0xFFFFFFFF,0x11111111);
  DEVICE_pinmuxControl(14,0xFFFFFFFF,0x88111111);
  DEVICE_pinmuxControl(15,0xFFFFFFFF,0x11888888);
  DEVICE_pinmuxControl(16,0xFFFFFFFF,0x11111111);
  DEVICE_pinmuxControl(17,0xFFFFFFFF,0x08111111);
  DEVICE_pinmuxControl(18,0xFFFFFFFF,0x11111111);
  DEVICE_pinmuxControl(19,0xFFFFFFFF,0x00000001);

  return E_PASS;
}

Uint32 DEVICE_SPIInit(Uint8 periphNum)
{
  if (periphNum == 0)
  {
    DEVICE_LPSCTransition(PSCNUM0, LPSC_SPI0, PD0, PSC_ENABLE);
    DEVICE_pinmuxControl(7,0xF0FFF000,0x10111000); // nSPI0_SCS[0], SPI0_CLK, SPI0_SIMO[0], SPI0_SOMI[0]
  }
  else if (periphNum == 1)
  {
    DEVICE_LPSCTransition(PSCNUM1, LPSC_SPI1, PD0, PSC_ENABLE);
    DEVICE_pinmuxControl(8,0x00000FFF,0x00000111); // SPI1_CLK, SPI1_SIMO[0], SPI1_SOMI[0]
    DEVICE_pinmuxControl(9,0x0000000F,0x00000001); // nSPI1_SCS[0]    
  }
  else
  {
    return E_FAIL;
  }
  
  return E_PASS;
}


/************************************************************
* Local Function Definitions                                *
************************************************************/


/***********************************************************
* End file                                                 *
***********************************************************/

//From here, Stanley Park added for compiling first, need to be significantly changed

Uint32 DEVICE_TIMER0Init()
{
  // Power-up the Timer
  //DEVICE_LPSCTransition(LPSC_TIMER0, PD0 ,PSC_ENABLE);

  // Put timer into reset
  //TIMER0->EMUMGT_CLKSPD = 0x00000003;
  //TIMER0->TCR   = 0x00000000;
  
  // Setup as 2 unchained 32-bit timers
  //TIMER0->TGCR  = 0x00000005;

  // Reset timers to zero 
  //TIMER0->TIM12 = 0x00000000;

  // Set period to 5 sec
  //TIMER0->PRD12 = 0x019BFCC0;

  return E_PASS;
}

void DEVICE_TIMER0Start(void)
{
  // Clear interrupt
  //AINTC->IRQ1   |=  0x00000001;

  // Put timer out in reset
  //TIMER0->TGCR  &= ~(0x00000001);
    
  // Reset the timer counter to zero
  //TIMER0->TIM12 = 0x00000000;

  // Set for one-time run
  //TIMER0->TCR   = 0x00000040;
    
  // Take timer out of reset
  //TIMER0->TGCR  = 0x00000005;
  
}

void DEVICE_TIMER0Stop(void)
{
  // Clear interrupt
  //AINTC->IRQ1   |=  0x00000001;

  // Put timer in reset
  //TIMER0->TCR   = 0x00000000;
  //TIMER0->TGCR  = 0x00000000;

  // Reset timer count to zero 
  //TIMER0->TIM12 = 0x00000000;
}

Uint32 DEVICE_TIMER0Status(void)
{
  //return ((AINTC->IRQ1)&0x1);
	return 0;
}
