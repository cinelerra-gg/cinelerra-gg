export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

TARGET_DIR := $(BINDIR)/doc

IMAGES = \
	arrow.png \
	autokeyframe.png \
	camera.png \
	channel.png \
	crop.png \
	cut.png \
	expandpatch_checked.png \
	eyedrop.png \
	fitautos.png \
	ibeam.png \
	left_justify.png \
	loadmode_cat.png \
	loadmode_nested.png \
	loadmode_newcat.png \
	loadmode_new.png \
	loadmode_newtracks.png \
	loadmode_none.png \
	loadmode_paste.png \
	loadmode_resource.png \
	magnify.png \
	mask.png \
	mutepatch_up.png \
	paste.png \
	projector.png \
	protect.png \
	recordpatch.png \
	record.png \
	rewind.png \
	show_meters.png \
	singleframe.png \
	titlesafe.png \
	toolwindow.png \
	top_justify.png \
	wrench.png \

OUTPUT := cinelerra.html

all: $(OUTPUT)

$(OUTPUT): $(IMAGES)
	makeinfo --html --no-split cinelerra.texi
#	makeinfo --html cinelerra.texi

$(IMAGES):
	cp $(PLUGINS)/theme_suv/data/$@ .

install:	all
	mkdir -p $(TARGET_DIR)
	cp -a *.png $(TARGET_DIR)/.
	cp -a ShellCmds*.* $(TARGET_DIR)/.
	cp -a cinelerra.html $(TARGET_DIR)/.
	cp -a Features5.pdf $(TARGET_DIR)/.
	cp -a shortcuts.html $(TARGET_DIR)/.
	cp -a RenderMux.sh $(TARGET_DIR)/.

clean:
	rm -f $(OUTPUT) $(IMAGES)

