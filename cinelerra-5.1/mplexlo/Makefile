export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

ifneq ($(WANT_LIBZMPEG),no)

$(shell mkdir -p $(OBJDIR))
OBJS = $(OBJDIR)/mplex.o
CC = gcc

CFLAGS += -I$(LIBZMPEG3)
CFLAGS += $(static_incs)

LIBS := $(LIBZMPEG3)/$(OBJDIR)/libzmpeg3.a
LIBS += $(libraries)

OUTPUT := $(OBJDIR)/mplexlo

$(OUTPUT) all: $(OBJS)
	@echo $(CXX) -o $(OUTPUT) $(OBJS)
	@$(CXX) -o $(OUTPUT) $(OBJS) $(LIBS)

$(OBJS):
	$(CXX) -c $(CFLAGS) $(subst $(OBJDIR)/,, $*.c) -o $*.o

clean:
	rm -rf $(OBJDIR)

install:
	cp $(OUTPUT) $(BINDIR)/.

$(OBJDIR)/mplex.o: mplex.c

else
#not WANT_LIBZMPEG
all:
rebuild:
utils:
clean:
install:
endif
