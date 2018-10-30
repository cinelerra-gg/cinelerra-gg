default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = moveobj

OBJS := \
	$(OBJDIR)/moveobj.o \
	$(OBJDIR)/moveobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/moveobj.o: moveobj.C moveobj.h moveobjwindow.h
$(OBJDIR)/moveobjwindow.o: moveobjwindow.C moveobj.h moveobjwindow.h

