APPLICATION = miot-pwrmon

# Add custom miot board directory
EXTERNAL_BOARD_DIRS=$(CURDIR)/miot-pcbs/RIOT/boards/

BOARD ?= miot-esp32

RIOTBASE ?= $(CURDIR)/RIOT

####################
# Module selection #
####################
USEMODULE += shell
USEMODULE += shell_cmds_default

#######################
# Development options #
#######################
DEVELHELP ?= 1
QUIET ?= 1
WERROR ?= 0

#include Makefile.versioning

#CFLAGS += -DMIOT_PWRMON_VERSION_STRING="\"$(VER_VERSION_STRING)\""

include $(RIOTBASE)/Makefile.include

.PHONY: release

release:
	cp $(BINDIR)/$(APPLICATION).elf $(APPLICATION)_$(VER_VERSION_STRING).elf
