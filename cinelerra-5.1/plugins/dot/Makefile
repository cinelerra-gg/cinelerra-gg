include ../../plugin_defs

OBJS = $(OBJDIR)/dot.o  \
	$(OBJDIR)/dotwindow.o \

PLUGIN = dot
CFLAGS += -I../libeffecttv
LFLAGS += ../libeffecttv/$(OBJDIR)/libeffecttv.a

include ../../plugin_config

$(OBJDIR)/dot.o: dot.C
$(OBJDIR)/dotwindow.o: dotwindow.C



