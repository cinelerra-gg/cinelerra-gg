include ../../plugin_defs

OBJS = $(OBJDIR)/interpolatevideo.o \
	$(OBJDIR)/interpolatewindow.o \
	$(OBJDIR)/opticflow.o \
	$(OBJDIR)/motioncache-hv.o \
	$(OBJDIR)/motionscan-hv.o

PLUGIN = interpolatevideo

include ../../plugin_config


$(OBJDIR)/interpolatevideo.o: interpolatevideo.C
$(OBJDIR)/interpolatewindow.o: interpolatewindow.C
$(OBJDIR)/opticflow.o: opticflow.C
$(OBJDIR)/motionscan-hv.o: motioncache-hv.C motioncache-hv.h
$(OBJDIR)/motionscan-hv.o: motionscan-hv.C motionscan-hv.h
