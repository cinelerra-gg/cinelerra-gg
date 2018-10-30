export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

ifneq ($(WANT_LIBZMPEG),no)

CC = gcc
NASM = nasm
USE_MMX ?= 0
DYNAMIC_FONTS ?= 1

ifeq ($(OBJDIR), alpha)
  ifneq ($(origin CFLAGS), environment)
    CFLAGS := -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
  endif
endif

ifeq ($(USE_MMX), 1)
  CFLAGS += -DHAVE_MMX
  ASMOBJS = $(OBJDIR)/video/mmxidct.o
  NASMOBJS = $(OBJDIR)/video/reconmmx.o
endif

CFLAGS += -I.  $(inc_a52dec)

OBJS = \
	$(OBJDIR)/audio/ac3.o \
	$(OBJDIR)/audio/dct.o \
	$(OBJDIR)/audio/huffman.o \
	$(OBJDIR)/audio/layer2.o \
	$(OBJDIR)/audio/layer3.o \
	$(OBJDIR)/audio/audio.o \
	$(OBJDIR)/audio/pcm.o \
	$(OBJDIR)/audio/synthesizers.o \
	$(OBJDIR)/audio/tables.o \
	$(OBJDIR)/libzmpeg3.o \
	$(OBJDIR)/atrack.o \
	$(OBJDIR)/bits.o \
	$(OBJDIR)/css.o \
	$(OBJDIR)/demux.o \
	$(OBJDIR)/strack.o \
	$(OBJDIR)/title.o \
	$(OBJDIR)/vtrack.o \
	$(OBJDIR)/zio.o \
	$(OBJDIR)/dvb.o \
	$(OBJDIR)/huf.o \
	$(OBJDIR)/xfont.o \
	$(OBJDIR)/video/getpicture.o \
	$(OBJDIR)/video/headers.o \
	$(OBJDIR)/video/idct.o \
	$(OBJDIR)/video/macroblocks.o \
	$(OBJDIR)/video/mmxtest.o \
	$(OBJDIR)/video/motion.o \
	$(OBJDIR)/video/cache.o \
	$(OBJDIR)/video/video.o \
	$(OBJDIR)/video/output.o \
	$(OBJDIR)/video/reconstruct.o \
	$(OBJDIR)/video/seek.o \
	$(OBJDIR)/video/slice.o \
	$(OBJDIR)/video/subtitle.o \
	$(OBJDIR)/video/cc.o \
	$(OBJDIR)/video/vlc.o \
	$(OBJDIR)/ifo.o \
	$(OBJDIR)/toc.o \
	$(OBJDIR)/mpeg3.o \

$(shell mkdir -p $(OBJDIR) )

DIRS := \
	$(OBJDIR)/audio \
	$(OBJDIR)/video
$(shell mkdir -p $(DIRS) )

OUTPUT = $(OBJDIR)/libzmpeg3.a
UTILS += $(OBJDIR)/zmpeg3toc
UTILS += $(OBJDIR)/zmpeg3show
UTILS += $(OBJDIR)/zmpeg3cat
UTILS += $(OBJDIR)/zmpeg3ifochk
UTILS += $(OBJDIR)/zmpeg3cc2txt

LIBS = -lm -lpthread

LIBS += $(lib_a52dec) $(lib_djbfft)

ifeq ($(DYNAMIC_FONTS), 1)
LIBS += -lX11
else
CFLAGS += -DBUILTIN_FONT_DATA 
OBJS += $(OBJDIR)/xfont1.o
endif

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) $(ASMOBJS) $(NASMOBJS) > $(OBJDIR)/objs)

all: $(OUTPUT) $(UTILS)

xfont1.C:	xfont.C
	$(CXX) `cat $(OBJDIR)/c_flags` -DWRITE_FONT_DATA -o $(OBJDIR)/xfont xfont.C \
		 $(LIBS) -lX11
	$(OBJDIR)/xfont > xfont1.C

b.out:	y.C $(OUTPUT)
	$(CXX) `pkg-config --cflags --libs gtk+-2.0` y.C ./$(OBJDIR)/libzmpeg3.a \
		$(CFLAGS) -O0 -g -o b.out -lasound $(LIBS)

$(OUTPUT): $(OBJS) $(ASMOBJS) $(NASMOBJS)
	ar rcs $(OUTPUT) `cat $(OBJDIR)/objs`


zmpeg3% $(OBJDIR)/zmpeg3%: $(OUTPUT) mpeg3%.C
	$(CXX) `cat $(OBJDIR)/c_flags` -o $(OBJDIR)/$(notdir $@) mpeg3$*.C $(OUTPUT) $(LIBS)

tags:
	ctags -R .

install: 
	cp $(UTILS) $(BINDIR)/.

clean:
	rm -rf $(OBJDIR) tags

$(OBJS): 
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.C) -o $*.o
$(ASMOBJS): 
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.S) -o $*.o
$(NASMOBJS): 
	$(NASM) -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o

$(OBJDIR)/%.o:		%.C
$(OBJDIR)/audio/%.o:	audio/%.C
$(OBJDIR)/video/%.o:	video/%.C

else
#not WANT_LIBZMPEG
all:
rebuild:
utils:
clean:
install:
endif
