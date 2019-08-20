#***********************************************
#   Makefile for use with Buildroot
#
# note:   $(CC) is passed in with the proper tool name.
#------------------------------------------------



CCFLAGS=-g -O0 -c


all: vars twsi_tool


twsi_tool: twsi_tool.o Makefile
	$(CC) $(LDFLAGS)  -o twsi_tool  twsi_tool.o

twsi_tool.o:  twsi_tool.c
	$(CC) $(CCFLAGS)  -o twsi_tool.o  twsi_tool.c


# need to remove .stamp_built and .stamp_target_installed to get folder ot re-build
clean:
	rm -rf *.o
	rm -rf twsi_tool
	rm -rf .stamp_built
	rm -rf .stamp_target_installed

clean_all:
	rm -rf *.o
	rm -rf twsi_tool
	rm -rf .stamp_built
	rm -rf .stamp_configured
	rm -rf .stamp_downloaded
	rm -rf .stamp_extracted
	rm -rf .stamp_patched
	rm -rf .stamp_target_installed





.phony: vars
vars:
	echo " CC:      $(CC)      "
	echo " CCFLAGS: $(CCFLAGS) "
	echo " LD:      $(LD)      "
	echo " LDFLAGS: $(LDFLAGS) "
