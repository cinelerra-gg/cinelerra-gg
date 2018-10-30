include ../../plugin_defs

OBJS = $(OBJDIR)/titler.o  \
	$(OBJDIR)/titlerwindow.o

PLUGIN = titler
CFLAGS += -Wall

ifneq ($(STATIC_LIBRARIES), y)
LFLAGS += -lfreetype
endif

include ../../plugin_config

$(OBJDIR)/titler.o: titler.C
$(OBJDIR)/titlerwindow.o: titlerwindow.C

