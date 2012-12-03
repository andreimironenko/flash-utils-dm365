all:
		$(MAKE) -C build TYPE=nand
		$(MAKE) -C build TYPE=nor
		$(MAKE) -C build TYPE=spi
		
##DM6441:
##		$(MAKE) -C build TYPE=nand DEVICE=DM6441
##		$(MAKE) -C build TYPE=nor DEVICE=DM6441
		
##DM6441_LV:
##		$(MAKE) -C build TYPE=nand DEVICE=DM6441_LV
##		$(MAKE) -C build TYPE=nor DEVICE=DM6441_LV

OMAP-L137:
		$(MAKE) -C build TYPE=nand DEVICE=OMAP-L137
		$(MAKE) -C build TYPE=nor DEVICE=OMAP-L137
		$(MAKE) -C build TYPE=spi DEVICE=OMAP-L137
		
clean:
		$(MAKE) -C build TYPE=nand clean
		$(MAKE) -C build TYPE=nor clean
		$(MAKE) -C build TYPE=spi clean
		ls
%::
		$(MAKE) -C build TYPE=nand $@
		$(MAKE) -C build TYPE=nor $@
		$(MAKE) -C build TYPE=spi $@
		
