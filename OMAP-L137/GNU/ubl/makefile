all:
		$(MAKE) -C build TYPE=NAND DEVICE=OMAP-L137
		
		$(MAKE) -C build TYPE=NOR DEVICE=OMAP-L137
		
    
clean:
		$(MAKE) -C build TYPE=NAND DEVICE=OMAP-L137 clean
	
		$(MAKE) -C build TYPE=NOR DEVICE=OMAP-L137 clean
	
%::
		$(MAKE) -C build TYPE=NAND DEVICE=OMAP-L137 $@

		$(MAKE) -C build TYPE=NOR DEVICE=OMAP-L137 $@

		
