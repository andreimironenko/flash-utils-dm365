all:
		$(MAKE) -C build TYPE=NAND
		$(MAKE) -C build TYPE=NOR
clean:
		$(MAKE) -C build TYPE=NAND clean
		$(MAKE) -C build TYPE=NOR clean
%::
		$(MAKE) -C build TYPE=NAND $@
		$(MAKE) -C build TYPE=NOR $@
		
