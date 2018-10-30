CFLAGS_ := $(CFLAGS)
CXXFLAGS_ := $(CXXFLAGS)
LDFLAGS_ := $(LDFLAGS)

export CFLAGS_ CXXFLAGS_ LDFLAGS_
unexport CFLAGS CXXFLAGS LDFLAGS

AM_MAKEFLAGS := -j$(WANT_JOBS)

AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = thirdparty libzmpeg3 mpeg2enc mplexlo \
	db guicast cinelerra plugins doc po db/utils

BUILT_SOURCES := bin
bin:
	mkdir -p bin bin/applications bin/pixmaps bin/lv2

clean-generic:
	rm -rf thirdparty/opencv*
	rm -rf bin

inst_sh := $(CURDIR)/inst.sh

install:	bin_install $(MAK_INSTALLS)

uninstall:	bin_uninstall $(MAK_UNINSTALLS)

.PHONY:	binstall cinstall cuninstall dinstall
export mkinstalldirs install_sh inst_sh

# install to bin
bin_install:	install-recursive
	cp -a COPYING README models Cinelerra_factory expanders.txt \
		ffmpeg msg info tips lv2_blacklist.txt bin/.
	sed -e 's/\<cin\>/$(WANT_CIN)/g' < image/cin.desktop \
		> "bin/applications/$(WANT_CIN).desktop"
	cp -a image/cin.svg "bin/pixmaps/$(WANT_CIN)".svg
	cp -a image/cin.xpm "bin/pixmaps/$(WANT_CIN)".xpm
	mkdir -p bin/plugins/picon/cinfinity
	for d in picon/*; do \
	  if [ $$d = picon/cinfinity ]; then continue; fi; \
	  cp -a $$d bin/plugins/picon/.; \
	done
	-cp -a db/utils/new_db "bin/cin_db"

bin_uninstall:
	rm -rf bin

# with system_build
sys_install: $(CIN_INSTALLS)
	cd bin; $(inst_sh) "$(DESTDIR)$(bindir)" "$(WANT_CIN)" bdwrite
	cd bin; $(inst_sh) "$(DESTDIR)$(datadir)/$(WANT_CIN)" \
		COPYING README models Cinelerra_factory expanders.txt \
		ffmpeg msg info tips doc
	cd bin/locale; $(inst_sh) "$(DESTDIR)$(localedir)" .
	cd bin/plugins; $(inst_sh) "$(DESTDIR)$(WANT_PLUGIN_DIR)" .
	cd bin/applications; $(inst_sh) "$(DESTDIR)$(datadir)/applications" .
	cd bin/pixmaps; $(inst_sh) "$(DESTDIR)$(datadir)/pixmaps" .

sys_uninstall: $(CIN_UNINSTALLS)
	rm -f "$(DESTDIR)$(bindir)/$(WANT_CIN)"
	rm -rf "$(DESTDIR)$(WANT_CINLIB_DIR)"
	rm -rf "$(DESTDIR)$(datadir)/$(WANT_CIN)"
	rm -rf "$(DESTDIR)$(localedir)"/*/"LC_MESSAGES/$(WANT_CIN).mo"
	rm -rf "$(DESTDIR)$(WANT_PLUGIN_DIR)"
	rm -f "$(DESTDIR)$(datadir)/applications/$(WANT_CIN).desktop"
	rm -f "$(DESTDIR)$(datadir)/pixmaps/$(WANT_CIN)".{svg,xpm}

# without system_build
dvl_install:
	cp Makefile.devel Makefile

dvl_uninstall:
	rm -f Makefile

# with-libzmpeg3
zmp_install:
	cd bin; $(inst_sh) "$(DESTDIR)$(bindir)" \
		zmpeg3{cat,cc2txt,ifochk,show,toc}
	cd bin; $(inst_sh) "$(DESTDIR)$(WANT_CINLIB_DIR)" \
		hveg2enc mpeg2enc mplex mplexlo

zmp_uninstall:
	rm -f "$(DESTDIR)$(bindir)"/zmpeg3{cat,cc2txt,ifochk,show,toc}

# with-commercial
com_install:
	cd bin; $(inst_sh) "$(DESTDIR)$(bindir)" cin_db
	cd bin; $(inst_sh) "$(DESTDIR)$(WANT_CINLIB_DIR)" cutads

com_uninstall:
	rm -f "$(DESTDIR)$(bindir)/cin_db"

# with-ladspa
lad_install:
	cd bin/ladspa; $(inst_sh) "$(DESTDIR)$(WANT_LADSPA_DIR)" .

lad_uninstall:
	rm -rf "$(DESTDIR)$(WANT_LADSPA_DIR)"

# with-lv2
lv2_install:
	cd bin; $(inst_sh) "$(DESTDIR)$(WANT_CINLIB_DIR)" lv2ui
	cd bin; $(inst_sh) "$(DESTDIR)$(datadir)/$(WANT_CIN)" \
		lv2 lv2_blacklist.txt

lv2_uninstall:

val-%:
	@echo $($(subst val-,,$@))

