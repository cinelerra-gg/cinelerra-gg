include ../../plugin_defs

OBJS = $(OBJDIR)/holo.o  \
	$(OBJDIR)/holowindow.o \

PLUGIN = holo
CFLAGS += -I../libeffecttv
LFLAGS += ../libeffecttv/$(OBJDIR)/libeffecttv.a

include ../../plugin_config

$(OBJDIR)/holo.o: holo.C
$(OBJDIR)/holowindow.o: holowindow.C
