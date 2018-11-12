export TOPDIR ?= $(CURDIR)/../..
include $(TOPDIR)/global_config

ifneq ($(WANT_COMMERCIAL),no)

$(shell mkdir -p $(OBJDIR))
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
CXXFLAGS:=-ggdb -I.. -Wall `cat $(OBJDIR)/c_flags`
LDLIBS:=../$(OBJDIR)/db.a
LDFLAGS:=

TARGETS:=clip_accesses \
	clip_data \
	clip_del \
	clip_dups \
	clip_ls \
	dbcounts \
	delallclips \
	frame_clips \
	framediff \
	frameweights \
	histeq \
	histplot \
	pbm2frame \
	pbm2key \
	timelinechk \
	clip_assets \
	clip_frames \
	clip_weights \
	dbtv \
	frame2pbm \
	framekey \
	new_db \
	root_info \
	dmp \
	cpdb \
	path_ls \
	xtv

#	add_path_pos \
#	dbcvt \

all:	$(TARGETS)

dbtv:	dbtv.C
	$(CXX) $(CXXFLAGS) $(LDFLAGS) dbtv.C $(LDLIBS) -lX11 -o $@

XTV_MEDIA_OBJS := $(TOPDIR)/cinelerra/$(OBJDIR)/mediadb.o
XTV_MEDIA_OBJS += $(TOPDIR)/cinelerra/$(OBJDIR)/filexml.o
XTV_MEDIA_OBJS += $(TOPDIR)/guicast/$(OBJDIR)/mutex.o
XTV_MEDIA_OBJS += $(TOPDIR)/guicast/$(OBJDIR)/bctrace.o

XTV_LIBS := $(TOPDIR)/libzmpeg3/$(OBJDIR)/libzmpeg3.a
XTV_LIBS += $(TOPDIR)/db/$(OBJDIR)/db.a
XTV_LIBS += $(libraries)

XTV_INCLUDES := -I$(TOPDIR) -I$(TOPDIR)/guicast
XTV_INCLUDES += $(static_incs)

xtv:	xtv.C $(XTV_MEDIA_OBJS)
	@echo c++ -pthread $(CXXFLAGS) xtv.C $(XTV_MEDIA_OBJS) -o xtv
	@c++ -pthread $(XTV_INCLUDES) $(CXXFLAGS) xtv.C $(XTV_MEDIA_OBJS) $(XTV_LIBS) -o xtv

install:

clean:
	rm -f $(TARGETS) *.o a.out

else
#not WANT_COMMERCIAL
all:
install:
clean:
endif
