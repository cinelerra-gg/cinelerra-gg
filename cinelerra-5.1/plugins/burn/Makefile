include ../../plugin_defs

OBJS = $(OBJDIR)/burn.o  \
	$(OBJDIR)/burnwindow.o \

PLUGIN = burn
CFLAGS += -I../libeffecttv
LFLAGS += ../libeffecttv/$(OBJDIR)/libeffecttv.a

include ../../plugin_config

$(OBJDIR)/burn.o: burn.C
$(OBJDIR)/burnwindow.o: burnwindow.C
