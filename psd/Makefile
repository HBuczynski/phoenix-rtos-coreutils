#
# Makefile for Phoenix-RTOS psd (Phoenix Serial Downloader)
#
# Copyright 2018, 2019 Phoenix Systems
#

$(PREFIX_PROG)psd: $(PREFIX_O)psd/psd.o $(PREFIX_O)psd/hid.o $(PREFIX_A)libusb-imx6ull.a
	$(LINK)
	
$(PREFIX_PROG)psd-old: $(PREFIX_O)psd/psd-old.o $(PREFIX_A)libusb-imx6ull.a
	$(LINK)

all: $(PREFIX_PROG_STRIPPED)psd-old $(PREFIX_PROG_STRIPPED)psd
