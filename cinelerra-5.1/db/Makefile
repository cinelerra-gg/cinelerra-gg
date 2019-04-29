export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

ifneq ($(WANT_COMMERCIAL),no)

$(shell mkdir -p $(OBJDIR))
LIBS := $(OBJDIR)/db.a ../libzmpeg3/$(OBJDIR)/libzmpeg3.a -lasound -lm -lX11
OUTPUT := $(OBJDIR)/db.a s.C

all: $(OUTPUT)

.PHONY:	rebuild
rebuild:
	make clean
	make -C utils clean
	make
	+make -C utils

.PHONY:	utils
utils:	$(OBJDIR)/db.a s.C
	+make -C utils

.PHONY:	clean
clean:
	make -C utils clean
	rm -rf a.out s.C tags $(OBJDIR)

.PHONY:	install
install:

OBJECTS := \
	$(OBJDIR)/tdb.o \

$(OBJECTS) $(OBJDIR)/xsch.o $(OBJDIR)/tst.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $(*F).C

$(OBJDIR)/db.a:	$(OBJECTS)
	ar rcs $@ $(OBJECTS)

s.C:	$(OBJDIR)/xsch sch.txt
	$(OBJDIR)/xsch < sch.txt

.o:
	$(CXX) $(LDFLAGS) -o $* $*.o $(LDLIBS)

#$(OBJDIR)/tdb.o:		tdb.C tdb.h
#	$(CXX) $(CXXFLAGS) -dD -E $(*F).C | grep -v '^# ' > z$(*F).C
#	$(CXX) $(CXXFLAGS) -c -o $@ z$(*F).C

$(OBJDIR)/tdb.o:	tdb.C tdb.h
$(OBJDIR)/xsch.o:	xsch.C tdb.h
$(OBJDIR)/xsch:		$(OBJDIR)/xsch.o

else
#not WANT_COMMERCIAL
all:
rebuild:
utils:
install:
clean:
endif
