default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = stylizeobj

OBJS := \
	$(OBJDIR)/stylizeobj.o \
	$(OBJDIR)/stylizeobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/stylizeobj.o: stylizeobj.C stylizeobj.h stylizeobjwindow.h
$(OBJDIR)/stylizeobjwindow.o: stylizeobjwindow.C stylizeobj.h stylizeobjwindow.h

