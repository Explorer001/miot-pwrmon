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
USEMODULE += saul_default

#######################
# Development options #
#######################
DEVELHELP ?= 1
QUIET ?= 1
WERROR ?= 0

SRC += main.c
SRC += $(wildcard src/*.c)

#include Makefile.versioning

#CFLAGS += -DMIOT_PWRMON_VERSION_STRING="\"$(VER_VERSION_STRING)\""

include $(RIOTBASE)/Makefile.include

.PHONY: release

release:
	cp $(BINDIR)/esp_bootloader/bootloader.bin bootloader_$(VER_VERSION_STRING).bin
	cp $(BINDIR)/partitions.bin partitions_$(VER_VERSION_STRING).bin
	cp $(BINDIR)/$(APPLICATION).elf.bin $(APPLICATION)_$(VER_VERSION_STRING).elf.bin
