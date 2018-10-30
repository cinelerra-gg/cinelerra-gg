default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = flowobj

OBJS := \
	$(OBJDIR)/flowobj.o \
	$(OBJDIR)/flowobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/flowobj.o: flowobj.C flowobj.h flowobjwindow.h
$(OBJDIR)/flowobjwindow.o: flowobjwindow.C flowobj.h flowobjwindow.h

