export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

ifneq ($(WANT_LIBZMPEG),no)

CC = gcc
CFLAGS += -I$(TOPDIR)/libzmpeg3

ifeq ($(OBJDIR), i686)
CFLAGS += -DX86_CPU
NASMOBJ = \
	$(OBJDIR)/bdist1_mmx.o \
	$(OBJDIR)/bdist2_mmx.o \
	$(OBJDIR)/dist2_mmx.o \
	$(OBJDIR)/fdct_mmx.o \
	$(OBJDIR)/idct_mmx.o \
	$(OBJDIR)/mblock_sad_mmx.o \
	$(OBJDIR)/mblock_sad_mmxe.o \
	$(OBJDIR)/mblockq_sad_mmxe.o \
	$(OBJDIR)/predcomp_mmx.o \
	$(OBJDIR)/predcomp_mmxe.o \
	$(OBJDIR)/predict_mmx.o \
	$(OBJDIR)/quant_mmx.o \

ASMOBJ = \
	$(OBJDIR)/mblock_sub44_sads.o \
	$(OBJDIR)/quantize_x86.o

#	$(OBJDIR)/mblock_sub44_sads.o \

endif

OBJ = \
	$(OBJDIR)/mpeg2enc.o \
	$(OBJDIR)/conform.o \
	$(OBJDIR)/cpu_accel.o \
	$(OBJDIR)/putseq.o \
	$(OBJDIR)/putpic.o \
	$(OBJDIR)/puthdr.o \
	$(OBJDIR)/putmpg.o \
	$(OBJDIR)/putvlc.o \
	$(OBJDIR)/putbits.o \
	$(OBJDIR)/motion.o \
	$(OBJDIR)/predict.o \
	$(OBJDIR)/readpic.o \
	$(OBJDIR)/transfrm.o \
	$(OBJDIR)/fdctdata.o \
	$(OBJDIR)/fdctref.o \
	$(OBJDIR)/idct.o \
	$(OBJDIR)/idctdata.o \
	$(OBJDIR)/quantize.o \
	$(OBJDIR)/ratectl.o \
	$(OBJDIR)/stats.o \
	$(OBJDIR)/writepic.o

LIBS := $(LIBZMPEG3)/$(OBJDIR)/libzmpeg3.a
LIBS += $(libraries)

HVEG2LIB = $(OBJDIR)/hveg2enc.a
HVEG2ENC = $(OBJDIR)/hveg2enc
HVEG2ENC_G = $(OBJDIR)/hveg2enc_g

$(shell mkdir -p $(OBJDIR))
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(HVEG2LIB)  $(OBJ) $(NASMOBJ) $(ASMOBJ) > $(OBJDIR)/objs)

$(HVEG2ENC):	$(OBJDIR)/hveg2enc.o $(HVEG2LIB)
	$(CXX) `cat $(OBJDIR)/c_flags` -o $@ $^ $(LIBS)
	$(if $(findstring -g,$(CFLAGS)),objcopy --only-keep-debug $(HVEG2ENC) $(HVEG2ENC_G))
	strip $(HVEG2ENC)

$(OBJDIR)/hveg2enc.o:	mpeg2enc.c
	$(CC) -c `cat $(OBJDIR)/c_flags` -Dmpeg2enc=main -o $@ $^

OUTPUT = $(HVEG2LIB) $(HVEG2ENC)

all: $(OUTPUT)

clean:
	rm -rf $(OBJDIR)

install:	$(OUTPUT)
	cp -a $(HVEG2ENC) $(BINDIR)/.

$(HVEG2LIB): $(OBJ) $(NASMOBJ) $(ASMOBJ)
	ar rcs `cat $(OBJDIR)/objs`

$(NASMOBJ):
	nasm -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o

$(OBJ) $(ASMOBJ):
	$(CC) -c `cat $(OBJDIR)/c_flags` $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(OBJDIR)/mpeg2enc.o: 			    mpeg2enc.c
$(OBJDIR)/conform.o: 			    conform.c
$(OBJDIR)/cpu_accel.o:  		    cpu_accel.c
$(OBJDIR)/putseq.o: 			    putseq.c
$(OBJDIR)/putpic.o: 			    putpic.c
$(OBJDIR)/puthdr.o: 			    puthdr.c
$(OBJDIR)/putmpg.o: 			    putmpg.c
$(OBJDIR)/putvlc.o: 			    putvlc.c
$(OBJDIR)/putbits.o: 			    putbits.c
$(OBJDIR)/motion.o: 			    motion.c
$(OBJDIR)/predict.o: 			    predict.c
$(OBJDIR)/readpic.o: 			    readpic.c
$(OBJDIR)/transfrm.o: 			    transfrm.c
$(OBJDIR)/fdctdata.o: 			    fdctdata.c
$(OBJDIR)/fdctref.o: 			    fdctref.c
$(OBJDIR)/idct.o: 			    idct.c
$(OBJDIR)/idctdata.o: 			    idctdata.c
$(OBJDIR)/quantize.o: 			    quantize.c
$(OBJDIR)/ratectl.o: 			    ratectl.c
$(OBJDIR)/stats.o: 			    stats.c
$(OBJDIR)/writepic.o: 			    writepic.c


$(OBJDIR)/bdist1_mmx.o: 		    bdist1_mmx.s
$(OBJDIR)/idct_mmx.o: 			    idct_mmx.s
$(OBJDIR)/mblockq_sad_mmxe.o: 		    mblockq_sad_mmxe.s
$(OBJDIR)/predcomp_mmxe.o: 		    predcomp_mmxe.s
$(OBJDIR)/bdist2_mmx.o: 		    bdist2_mmx.s
$(OBJDIR)/mblock_sad_mmx.o: 	 	    mblock_sad_mmx.s
$(OBJDIR)/predict_mmx.o: 		    predict_mmx.s
$(OBJDIR)/dist2_mmx.o:	    		    dist2_mmx.s
$(OBJDIR)/mblock_sad_mmxe.o:		    mblock_sad_mmxe.s
$(OBJDIR)/predcomp_mmx.o:		    predcomp_mmx.s
$(OBJDIR)/quant_mmx.o:	    		    quant_mmx.s
$(OBJDIR)/fdct_mmx.o:			    fdct_mmx.s

#$(OBJDIR)/mblock_sub44_sads.o: 	 	    mblock_sub44_sads.c
$(OBJDIR)/quantize_x86.o:		    quantize_x86.c

val-%:
	@echo $($(subst val-,,$@))

else
#not WANT_LIBZMPEG
all:
rebuild:
utils:
clean:
install:
endif
