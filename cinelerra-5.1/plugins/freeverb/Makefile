include ../../plugin_defs

PLUGIN = freeverb
OBJS := $(OBJDIR)/freeverb.o
DIRS := $(OBJDIR)/Components
FREEVERBOBJS := \
	$(OBJDIR)/Components/allpass.o \
	$(OBJDIR)/Components/comb.o \
	$(OBJDIR)/Components/revmodel.o

CFLAGS += -IComponents
LFLAGS += $(FREEVERBOBJS)

include ../../plugin_config

$(FREEVERBOBJS):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.cpp) -o $*.o

$(FREEVERBOBJS): $(DIRS)
$(OUTPUT): $(FREEVERBOBJS)

$(DIRS):
	mkdir -p $@



$(OBJDIR)/freeverb.o: freeverb.C
$(OBJDIR)/Components/allpass.o:  Components/allpass.cpp
$(OBJDIR)/Components/comb.o: 	 Components/comb.cpp
$(OBJDIR)/Components/revmodel.o: Components/revmodel.cpp








