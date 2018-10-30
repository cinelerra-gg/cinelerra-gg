include ../../plugin_defs

OBJS =  $(OBJDIR)/bluebanana.o
OBJS += $(OBJDIR)/bluebananaconfig.o
OBJS += $(OBJDIR)/bluebananaslider.o
OBJS += $(OBJDIR)/bluebananawindow.o
OBJS += $(OBJDIR)/bluebananalookups.o
OBJS += $(OBJDIR)/bluebananaengine.o

PLUGIN = bluebanana

include ../../plugin_config

$(OBJDIR)/bluebanana.o:			bluebanana.C
$(OBJDIR)/bluebananaconfig.o:		bluebananaconfig.C
$(OBJDIR)/bluebananaslider.o:		bluebananaslider.C
$(OBJDIR)/bluebananawindow.o:		bluebananawindow.C bluebananacolor.c
$(OBJDIR)/bluebananalookups.o:		bluebananalookups.C
$(OBJDIR)/bluebananaengine.o:		bluebananaengine.C bluebananacolor.c

