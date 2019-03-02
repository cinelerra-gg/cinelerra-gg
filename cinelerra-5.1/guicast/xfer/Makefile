export TOPDIR ?= $(CURDIR)/../..
include $(TOPDIR)/global_config

$(shell mkdir -p $(OBJDIR) )
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)

OUTPUT := $(OBJDIR)/xfer.stamp

all:	$(OUTPUT)

clean:
	rm -rf $(OBJDIR) xfer*.[Ch]

BFLAGS:=-Ofast

$(OBJDIR)/%.o:          %.C
	$(CXX) -I.. `cat $(OBJDIR)/c_flags` $(BFLAGS) -DMSGQUAL=$* -c $< -o $@

$(OUTPUT): $(patsubst %.C,$(OBJDIR)/%.o,$(wildcard *.C))
	touch $@
