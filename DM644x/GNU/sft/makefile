all:
		$(MAKE) -C build TYPE=NAND
		$(MAKE) -C build TYPE=NOR
DM6441:
		$(MAKE) -C build TYPE=NAND DEVICE=DM6441
		$(MAKE) -C build TYPE=NOR DEVICE=DM6441
		
DM6441_LV:
		$(MAKE) -C build TYPE=NAND DEVICE=DM6441_LV
		$(MAKE) -C build TYPE=NOR DEVICE=DM6441_LV
		
clean:
		$(MAKE) -C build TYPE=NAND clean
		$(MAKE) -C build TYPE=NOR clean
%::
		$(MAKE) -C build TYPE=NAND $@
		$(MAKE) -C build TYPE=NOR $@
		
