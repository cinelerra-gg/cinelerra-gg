include ../../plugin_defs

PLUGIN = chromakeyhsv
OUTPUT_BINS := $(OBJDIR)/chromakey_sl.o
OBJS := $(OBJDIR)/chromakey.o $(SHADER)

include ../../plugin_config

$(OBJDIR)/chromakey.o: chromakey.C
$(OUTPUT_BINS): chromakey.sl
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap -s chromakey_sl.o ../chromakey.sl

