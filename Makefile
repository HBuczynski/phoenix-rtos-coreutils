#
# Makefile for psh
#
# Copyright 2018, 2019 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

#TARGET ?= ia32-qemu
#TARGET ?= armv7-stm32
TARGET ?= arm-imx6ull

TOPDIR := $(CURDIR)
BUILD_PREFIX ?= ../_build/$(TARGET)
BUILD_PREFIX := $(abspath $(BUILD_PREFIX))
BUILD_DIR ?= $(BUILD_PREFIX)/$(notdir $(TOPDIR))
BUILD_DIR := $(abspath $(BUILD_DIR))

# Compliation options for various architectures
TARGET_FAMILY = $(firstword $(subst -, ,$(TARGET)-))
include Makefile.$(TARGET_FAMILY)

# build artifacts dir
CURR_SUFFIX := $(patsubst $(TOPDIR)/%,%,$(abspath $(CURDIR))/)
PREFIX_O := $(BUILD_DIR)/$(CURR_SUFFIX)

# target install paths, can be provided exterally
PREFIX_A ?= $(BUILD_PREFIX)/lib/
PREFIX_H ?= $(BUILD_PREFIX)/include/
PREFIX_PROG ?= $(BUILD_PREFIX)/prog/
PREFIX_PROG_STRIPPED ?= $(BUILD_PREFIX)/prog.stripped/

CFLAGS += -I"$(PREFIX_H)"
LDFLAGS += -L"$(PREFIX_A)"

# add include path for auto-generated files
CFLAGS += -I"$(BUILD_DIR)/$(CURR_SUFFIX)"

ARCH =  $(SIL)@mkdir -p $(@D); \
	(printf "AR  %-24s\n" "$(@F)"); \
	$(AR) $(ARFLAGS) $@ $^ 2>/dev/null

LINK = $(SIL)mkdir -p $(@D); \
	(printf "LD  %-24s\n" "$(@F)"); \
	$(LD) $(LDFLAGS) -o "$@"  $^ $(LDLIBS)
	
HEADER = $(SIL)mkdir -p $(@D); \
	(printf "HEADER %-24s\n" "$<"); \
	cp -pR "$<" "$@"

$(PREFIX_O)%.o: %.c
	@mkdir -p $(@D)
	$(SIL)(printf "CC  %-24s\n" "$<")
	$(SIL)$(CC) -c $(CFLAGS) "$<" -o "$@"
	$(SIL)$(CC) -M  -MD -MP -MF $(PREFIX_O)$*.c.d -MT "$@" $(CFLAGS) $<

$(PREFIX_O)%.o: %.S
	@mkdir -p $(@D)
	$(SIL)(printf "ASM %s/%-24s\n" "$(notdir $(@D))" "$<")
	$(SIL)$(CC) -c $(CFLAGS) "$<" -o "$@"
	$(SIL)$(CC) -M  -MD -MP -MF $(PREFIX_O)$*.S.d -MT "$@" $(CFLAGS) $<
	
$(PREFIX_PROG_STRIPPED)%: $(PREFIX_PROG)%
	@mkdir -p $(@D)
	@(printf "STR %-24s\n" "$(@F)")
	$(SIL)$(STRIP) -o $@ $<

$(PREFIX_PROG)psh: $(PREFIX_O)psh.o
	$(LINK)

all: $(PREFIX_PROG_STRIPPED)psh

.PHONY: clean
clean:
	rm -rf $(PREFIX_PROG)psh
	rm -rf $(PREFIX_PROG_STRIPPED)psh
	rm -rf $(PREFIX_O)*.o