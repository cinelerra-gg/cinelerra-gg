default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = gaborobj

OBJS := \
	$(OBJDIR)/gaborobj.o \
	$(OBJDIR)/gaborobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/gaborobj.o: gaborobj.C gaborobj.h gaborobjwindow.h
$(OBJDIR)/gaborobjwindow.o: gaborobjwindow.C gaborobj.h gaborobjwindow.h

