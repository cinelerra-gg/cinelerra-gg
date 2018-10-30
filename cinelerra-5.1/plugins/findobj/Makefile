default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = findobj

OBJS := \
	$(OBJDIR)/findobj.o \
	$(OBJDIR)/findobjwindow.o \

want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/findobj.o: findobj.C findobj.h findobjwindow.h
$(OBJDIR)/findobjwindow.o: findobjwindow.C findobj.h findobjwindow.h

