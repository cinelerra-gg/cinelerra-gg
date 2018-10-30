include ../../plugin_defs

OBJS = $(OBJDIR)/aging.o  \
	$(OBJDIR)/agingwindow.o \

PLUGIN = aging
CFLAGS += -I../libeffecttv
LFLAGS += ../libeffecttv/$(OBJDIR)/libeffecttv.a

include ../../plugin_config

$(OBJDIR)/aging.o: aging.C
$(OBJDIR)/agingwindow.o: agingwindow.C
