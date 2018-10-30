export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

OBJS := $(foreach i,de es eu fr it nb pt sl hi ko vi zh el ja uk ru, $(i).mo)
MOBJS := $(foreach i,$(OBJS),$(BINDIR)/locale/$(basename $(i))/LC_MESSAGES/$(WANT_CIN).mo)

all: $(OBJS)

install: $(MOBJS)

clean:
	rm -f $(OBJS)

%.mo: %.po
	msgfmt -c -o $@ $*.po

$(BINDIR)/locale/%/LC_MESSAGES/$(WANT_CIN).mo: %.mo
	mkdir -p $(dir $@)
	cp $*.mo $@

