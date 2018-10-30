default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = puzzleobj

OBJS := \
	$(OBJDIR)/puzzleobj.o \
	$(OBJDIR)/puzzleobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/puzzleobj.o: puzzleobj.C puzzleobj.h puzzleobjwindow.h
$(OBJDIR)/puzzleobjwindow.o: puzzleobjwindow.C puzzleobj.h puzzleobjwindow.h

