#############################################################
# Makefile for TI Serial Flasher Host (SFH) project.        #
#   Generates the Host executable file that is used in      #
#   in conjunction with the SFT to send and boot over UART. #
#   This executable is dependent on the SFT binary having   #
#   been generated first.                                   #
#############################################################
# Auth: Stanley Park
#	    
PROGRAM:=sfh_ais

MONOCOMPILE=gmcs
DOTNETCOMPILE=csc
REMOTESOURCES=$(PROGRAM).cs sfh_AIS_Util.cs Debug.cs CRC32.cs EmbeddedFileIO.cs FileIO.cs SerialIO.cs ConsoleUtility.cs



OBJECTS:=$(patsubst %.cs,%.module,$(REMOTESOURCES))
EXECUTABLE:=../$(PROGRAM).exe
INCPATH:= ../sfh_ais/inc
ICONPATH:=../sfh_ais/ti.ico

# Target definitions
all: $(EXECUTABLE) $(OBJECTS)

.PHONY : clean
clean:
		-rm -f -v $(EXECUTABLE) $(OBJECTS)

$(EXECUTABLE): $(OBJECTS) 
ifeq ($(shell uname),Linux)
		@$(MONOCOMPILE) -t:exe -out:$(EXECUTABLE) $(OBJECTS)
else
		@$(DOTNETCOMPILE) /t:exe /out:$(EXECUTABLE) /win32icon:$(ICONPATH) $(OBJECTS)
endif

%.module: %.cs
		@cp $< $@


vpath %.cs $(INCPATH)/ 
# :$(COMMONPATH)/sfh/:../../Common/src/


