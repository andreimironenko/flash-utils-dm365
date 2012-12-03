#############################################################
# Makefile for UBL CCS project(s).                          #
#############################################################
#	    
PROGRAM:=UBL
DEVSTRING:=DM36x
FLASHTYPES=NAND NOR SPI
SPIBIN=./$(PROGRAM)_$(DEVSTRING)_SPI.bin
NANDBIN=./$(PROGRAM)_$(DEVSTRING)_NAND.bin
NORBIN=./$(PROGRAM)_$(DEVSTRING)_NOR.bin


# Target definitions
.PHONY : all clean
all: 
		timake $(PROGRAM)_nandboot.pjt BOOT_NAND
		timake $(PROGRAM)_norboot.pjt BOOT_NOR
		timake $(PROGRAM)_spiboot.pjt BOOT_SPI

clean:
		timake $(PROGRAM)_nandboot.pjt BOOT_NAND -clean
		timake $(PROGRAM)_norboot.pjt BOOT_NOR -clean
		timake $(PROGRAM)_spiboot.pjt BOOT_SPI -clean
		rm -f $(NANDBIN) $(NORBIN) $(SPIBIN)


