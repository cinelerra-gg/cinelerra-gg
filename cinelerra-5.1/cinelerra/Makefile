export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

OBJS := \
	$(OBJDIR)/aattachmentpoint.o \
	$(OBJDIR)/aautomation.o \
	$(OBJDIR)/aboutprefs.o \
	$(OBJDIR)/adeviceprefs.o \
	$(OBJDIR)/aedit.o \
	$(OBJDIR)/aedits.o \
	$(OBJDIR)/affine.o \
	$(OBJDIR)/amodule.o \
	$(OBJDIR)/androidcontrol.o \
	$(OBJDIR)/apatchgui.o \
	$(OBJDIR)/apluginarray.o \
	$(OBJDIR)/aplugin.o \
	$(OBJDIR)/apluginset.o \
	$(OBJDIR)/appearanceprefs.o \
	$(OBJDIR)/arender.o \
	$(OBJDIR)/assetedit.o \
	$(OBJDIR)/asset.o \
	$(OBJDIR)/assetpopup.o \
	$(OBJDIR)/assetremove.o \
	$(OBJDIR)/assets.o \
	$(OBJDIR)/atrack.o \
	$(OBJDIR)/attachmentpoint.o \
	$(OBJDIR)/audio1394.o \
	$(OBJDIR)/audioalsa.o \
	$(OBJDIR)/audiodevice.o \
	$(OBJDIR)/audiodvb.o \
	$(OBJDIR)/audioesound.o \
	$(OBJDIR)/audioidevice.o \
	$(OBJDIR)/audioodevice.o \
	$(OBJDIR)/audiooss.o \
	$(OBJDIR)/audiov4l2mpeg.o \
	$(OBJDIR)/autoconf.o \
	$(OBJDIR)/automation.o \
	$(OBJDIR)/auto.o \
	$(OBJDIR)/autos.o \
	$(OBJDIR)/avc1394control.o \
	$(OBJDIR)/avc1394transport.o \
	$(OBJDIR)/awindowgui.o \
	$(OBJDIR)/awindow.o \
	$(OBJDIR)/batch.o \
	$(OBJDIR)/batchrender.o \
	$(OBJDIR)/bdcreate.o \
	$(OBJDIR)/binfolder.o \
	$(OBJDIR)/bitspopup.o \
	$(OBJDIR)/brender.o \
	$(OBJDIR)/browsebutton.o \
	$(OBJDIR)/byteorderpopup.o \
	$(OBJDIR)/cachebase.o \
	$(OBJDIR)/cache.o \
	$(OBJDIR)/canvas.o \
	$(OBJDIR)/canvastools.o \
	$(OBJDIR)/channeldb.o \
	$(OBJDIR)/channeledit.o \
	$(OBJDIR)/channelinfo.o \
	$(OBJDIR)/channel.o \
	$(OBJDIR)/channelpicker.o \
	$(OBJDIR)/chantables.o \
	$(OBJDIR)/clipedit.o \
	$(OBJDIR)/clipedls.o \
	$(OBJDIR)/clippopup.o \
	$(OBJDIR)/colorpicker.o \
	$(OBJDIR)/commonrender.o \
	$(OBJDIR)/confirmquit.o \
	$(OBJDIR)/confirmsave.o \
	$(OBJDIR)/cpanel.o \
	$(OBJDIR)/cplayback.o \
	$(OBJDIR)/ctimebar.o \
	$(OBJDIR)/ctracking.o \
	$(OBJDIR)/cursor.o \
	$(OBJDIR)/cwindowgui.o \
	$(OBJDIR)/cwindow.o \
	$(OBJDIR)/cwindowtool.o \
	$(OBJDIR)/dcoffset.o \
	$(OBJDIR)/deleteallindexes.o \
	$(OBJDIR)/device1394input.o \
	$(OBJDIR)/device1394output.o \
	$(OBJDIR)/devicedvbinput.o \
	$(OBJDIR)/devicev4l2base.o \
	$(OBJDIR)/devicev4l2input.o \
	$(OBJDIR)/dragcheckbox.o \
	$(OBJDIR)/drivesync.o \
	$(OBJDIR)/dvdcreate.o \
	$(OBJDIR)/edithandles.o \
	$(OBJDIR)/editlength.o \
	$(OBJDIR)/edit.o \
	$(OBJDIR)/editpanel.o \
	$(OBJDIR)/editpopup.o \
	$(OBJDIR)/edits.o \
	$(OBJDIR)/edl.o \
	$(OBJDIR)/edlsession.o \
	$(OBJDIR)/effectlist.o \
	$(OBJDIR)/exportedl.o \
	$(OBJDIR)/fadeengine.o \
	$(OBJDIR)/ffmpeg.o \
	$(OBJDIR)/filebaseaudio.o \
	$(OBJDIR)/filebase.o \
	$(OBJDIR)/filebaseulaw.o \
	$(OBJDIR)/filecr2.o \
	$(OBJDIR)/filedv.o \
	$(OBJDIR)/fileexr.o \
	$(OBJDIR)/fileffmpeg.o \
	$(OBJDIR)/fileflac.o \
	$(OBJDIR)/fileformat.o \
	$(OBJDIR)/filegif.o \
	$(OBJDIR)/filejpeg.o \
	$(OBJDIR)/filelist.o \
	$(OBJDIR)/file.o \
	$(OBJDIR)/filepng.o \
	$(OBJDIR)/fileppm.o \
	$(OBJDIR)/filescene.o \
	$(OBJDIR)/filesndfile.o \
	$(OBJDIR)/filetga.o \
	$(OBJDIR)/filethread.o \
	$(OBJDIR)/filetiff.o \
	$(OBJDIR)/filexml.o \
	$(OBJDIR)/floatauto.o \
	$(OBJDIR)/floatautos.o \
	$(OBJDIR)/folderlistmenu.o \
	$(OBJDIR)/forkbase.o \
	$(OBJDIR)/formatcheck.o \
	$(OBJDIR)/formatpopup.o \
	$(OBJDIR)/formatpresets.o \
	$(OBJDIR)/formattools.o \
	$(OBJDIR)/fourier.o \
	$(OBJDIR)/framecache.o \
	$(OBJDIR)/garbage.o \
	$(OBJDIR)/gwindowgui.o \
	$(OBJDIR)/gwindow.o \
	$(OBJDIR)/iec61883input.o \
	$(OBJDIR)/iec61883output.o \
	$(OBJDIR)/indexable.o \
	$(OBJDIR)/indexfile.o \
	$(OBJDIR)/indexstate.o \
	$(OBJDIR)/indexthread.o \
	$(OBJDIR)/intauto.o \
	$(OBJDIR)/intautos.o \
	$(OBJDIR)/interfaceprefs.o \
	$(OBJDIR)/interlacemodes.o \
	$(OBJDIR)/keyframegui.o \
	$(OBJDIR)/keyframehandles.o \
	$(OBJDIR)/keyframe.o \
	$(OBJDIR)/keyframepopup.o \
	$(OBJDIR)/keyframes.o \
	$(OBJDIR)/labeledit.o \
	$(OBJDIR)/labelpopup.o \
	$(OBJDIR)/labelnavigate.o \
	$(OBJDIR)/labels.o \
	$(OBJDIR)/levelwindowgui.o \
	$(OBJDIR)/levelwindow.o \
	$(OBJDIR)/libdv.o \
	$(OBJDIR)/libmjpeg.o \
	$(OBJDIR)/loadbalance.o \
	$(OBJDIR)/loadfile.o \
	$(OBJDIR)/loadmode.o \
	$(OBJDIR)/localsession.o \
	$(OBJDIR)/mainclock.o \
	$(OBJDIR)/maincursor.o \
	$(OBJDIR)/mainerror.o \
	$(OBJDIR)/mainindexes.o \
	$(OBJDIR)/mainmenu.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/mainprogress.o \
	$(OBJDIR)/mainsession.o \
	$(OBJDIR)/mainundo.o \
	$(OBJDIR)/manualgoto.o \
	$(OBJDIR)/maskauto.o \
	$(OBJDIR)/maskautos.o \
	$(OBJDIR)/maskengine.o \
	$(OBJDIR)/mbuttons.o \
	$(OBJDIR)/menuaeffects.o \
	$(OBJDIR)/menuattacheffect.o \
	$(OBJDIR)/menuattachtransition.o \
	$(OBJDIR)/menueditlength.o \
	$(OBJDIR)/menueffects.o \
	$(OBJDIR)/menutransitionlength.o \
	$(OBJDIR)/menuveffects.o \
	$(OBJDIR)/meterhistory.o \
	$(OBJDIR)/meterpanel.o \
	$(OBJDIR)/module.o \
	$(OBJDIR)/mtimebar.o \
	$(OBJDIR)/mwindowedit.o \
	$(OBJDIR)/mwindowgui.o \
	$(OBJDIR)/mwindowmove.o \
	$(OBJDIR)/mwindow.o \
	$(OBJDIR)/new.o \
	$(OBJDIR)/overlaydirect.o \
	$(OBJDIR)/overlayframe.o \
	$(OBJDIR)/overlaynearest.o \
	$(OBJDIR)/overlaysample.o \
	$(OBJDIR)/packagedispatcher.o \
	$(OBJDIR)/packagerenderer.o \
	$(OBJDIR)/packagingengine.o \
	$(OBJDIR)/panauto.o \
	$(OBJDIR)/panautos.o \
	$(OBJDIR)/panedividers.o \
	$(OBJDIR)/patchbay.o \
	$(OBJDIR)/patchgui.o \
	$(OBJDIR)/performanceprefs.o \
	$(OBJDIR)/picture.o \
	$(OBJDIR)/playabletracks.o \
	$(OBJDIR)/playback3d.o \
	$(OBJDIR)/playbackconfig.o \
	$(OBJDIR)/playbackengine.o \
	$(OBJDIR)/playbackprefs.o \
	$(OBJDIR)/playtransport.o \
	$(OBJDIR)/pluginaclientlad.o \
	$(OBJDIR)/pluginaclient.o \
	$(OBJDIR)/pluginarray.o \
	$(OBJDIR)/pluginclient.o \
	$(OBJDIR)/plugindialog.o \
	$(OBJDIR)/pluginfclient.o \
	$(OBJDIR)/pluginlv2.o \
	$(OBJDIR)/pluginlv2config.o \
	$(OBJDIR)/pluginlv2client.o \
	$(OBJDIR)/pluginlv2gui.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/pluginpopup.o \
	$(OBJDIR)/pluginserver.o \
	$(OBJDIR)/pluginset.o \
	$(OBJDIR)/plugintclient.o \
	$(OBJDIR)/plugintoggles.o \
	$(OBJDIR)/pluginvclient.o \
	$(OBJDIR)/preferences.o \
	$(OBJDIR)/preferencesthread.o \
	$(OBJDIR)/presets.o \
	$(OBJDIR)/probeprefs.o \
	$(OBJDIR)/proxy.o \
	$(OBJDIR)/proxypopup.o \
	$(OBJDIR)/question.o \
	$(OBJDIR)/quit.o \
	$(OBJDIR)/recconfirmdelete.o \
	$(OBJDIR)/recordableatracks.o \
	$(OBJDIR)/recordablevtracks.o \
	$(OBJDIR)/recordaudio.o \
	$(OBJDIR)/recordbatches.o \
	$(OBJDIR)/recordconfig.o \
	$(OBJDIR)/recordgui.o \
	$(OBJDIR)/recordlabel.o \
	$(OBJDIR)/recordmonitor.o \
	$(OBJDIR)/record.o \
	$(OBJDIR)/recordprefs.o \
	$(OBJDIR)/recordscopes.o \
	$(OBJDIR)/recordthread.o \
	$(OBJDIR)/recordtransport.o \
	$(OBJDIR)/recordvideo.o \
	$(OBJDIR)/remotecontrol.o \
	$(OBJDIR)/removefile.o \
	$(OBJDIR)/renderengine.o \
	$(OBJDIR)/renderfarmclient.o \
	$(OBJDIR)/renderfarm.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/renderprofiles.o \
	$(OBJDIR)/resample.o \
	$(OBJDIR)/rescale.o \
	$(OBJDIR)/resizetrackthread.o \
	$(OBJDIR)/resourcepixmap.o \
	$(OBJDIR)/resourcethread.o \
	$(OBJDIR)/samplescroll.o \
	$(OBJDIR)/samples.o \
	$(OBJDIR)/savefile.o \
	$(OBJDIR)/scenegraph.o \
	$(OBJDIR)/scopewindow.o \
	$(OBJDIR)/setformat.o \
	$(OBJDIR)/sha1.o \
	$(OBJDIR)/sharedlocation.o \
	$(OBJDIR)/shbtnprefs.o \
	$(OBJDIR)/shmemory.o \
	$(OBJDIR)/shuttle.o \
	$(OBJDIR)/sighandler.o \
	$(OBJDIR)/signalstatus.o \
	$(OBJDIR)/splashgui.o \
	$(OBJDIR)/statusbar.o \
	$(OBJDIR)/strack.o \
	$(OBJDIR)/swindow.o \
	$(OBJDIR)/theme.o \
	$(OBJDIR)/threadexec.o \
	$(OBJDIR)/threadloader.o \
	$(OBJDIR)/timebar.o \
	$(OBJDIR)/timeentry.o \
	$(OBJDIR)/timelinepane.o \
	$(OBJDIR)/tipwindow.o \
	$(OBJDIR)/trackcanvas.o \
	$(OBJDIR)/tracking.o \
	$(OBJDIR)/track.o \
	$(OBJDIR)/trackpopup.o \
	$(OBJDIR)/trackscroll.o \
	$(OBJDIR)/tracksedit.o \
	$(OBJDIR)/tracks.o \
	$(OBJDIR)/transitionhandles.o \
	$(OBJDIR)/transition.o \
	$(OBJDIR)/transitionpopup.o \
	$(OBJDIR)/transportque.o \
	$(OBJDIR)/tunerserver.o \
	$(OBJDIR)/undostack.o \
	$(OBJDIR)/vattachmentpoint.o \
	$(OBJDIR)/vautomation.o \
	$(OBJDIR)/vdevice1394.o \
	$(OBJDIR)/vdevicebase.o \
	$(OBJDIR)/vdevicedvb.o \
	$(OBJDIR)/vdeviceprefs.o \
	$(OBJDIR)/vdevicev4l2jpeg.o \
	$(OBJDIR)/vdevicev4l2mpeg.o \
	$(OBJDIR)/vdevicev4l2.o \
	$(OBJDIR)/vdevicex11.o \
	$(OBJDIR)/vedit.o \
	$(OBJDIR)/vedits.o \
	$(OBJDIR)/videodevice.o \
	$(OBJDIR)/viewmenu.o \
	$(OBJDIR)/virtualaconsole.o \
	$(OBJDIR)/virtualanode.o \
	$(OBJDIR)/virtualconsole.o \
	$(OBJDIR)/virtualnode.o \
	$(OBJDIR)/virtualvconsole.o \
	$(OBJDIR)/virtualvnode.o \
	$(OBJDIR)/vmodule.o \
	$(OBJDIR)/vpatchgui.o \
	$(OBJDIR)/vplayback.o \
	$(OBJDIR)/vpluginarray.o \
	$(OBJDIR)/vplugin.o \
	$(OBJDIR)/vpluginset.o \
	$(OBJDIR)/vrender.o \
	$(OBJDIR)/vtimebar.o \
	$(OBJDIR)/vtracking.o \
	$(OBJDIR)/vtrack.o \
	$(OBJDIR)/vwindowgui.o \
	$(OBJDIR)/vwindow.o \
	$(OBJDIR)/wavecache.o \
	$(OBJDIR)/wwindow.o \
	$(OBJDIR)/zoombar.o \
	$(OBJDIR)/zoompanel.o \
	$(OBJDIR)/zwindow.o \
	$(OBJDIR)/zwindowgui.o \

ifneq ($(WANT_LIBZMPEG),no)
OBJS += \
	$(OBJDIR)/audiompeg.o \
	$(OBJDIR)/devicempeginput.o \
	$(OBJDIR)/filempeg.o \
	$(OBJDIR)/vdevicempeg.o \

endif
ifneq ($(WANT_COMMERCIAL),no)
OBJS += \
	$(OBJDIR)/commercials.o \
	$(OBJDIR)/dbwindow.o \
	$(OBJDIR)/filedb.o \
	$(OBJDIR)/mediadb.o \

endif
ifneq ($(WANT_CIN_3RDPARTY),no)
OBJS += \
	$(OBJDIR)/fileac3.o \

endif

#	$(OBJDIR)/dvbtune.o \
#	$(OBJDIR)/renderfarmfsclient.o \
#	$(OBJDIR)/renderfarmfsserver.o \

GCC ?= gcc
DCRAW := $(OBJDIR)/dcraw.o
THEME_DATA := $(OBJDIR)/theme_data.o

OUTPUT = $(BINDIR)/$(WANT_CIN)
OUTPUT_G = $(OBJDIR)/$(WANT_CIN).debuginfo

LIBRARIES := \
	$(GUICAST)/$(OBJDIR)/libguicast.a \

ifneq ($(WANT_LIBZMPEG),no)
LIBRARIES += \
	$(MPEG2ENC)/$(OBJDIR)/hveg2enc.a \
	$(LIBZMPEG3)/$(OBJDIR)/libzmpeg3.a \

endif
ifneq ($(WANT_COMMERCIAL),no)
LIBRARIES += \
	$(TDB)/$(OBJDIR)/db.a \

endif

LIBS := $(LIBRARIES)

ifneq ($(WANT_CIN_3RDPARTY),no)
LIBS += $(libraries)
else

LIBS += -lpthread -lm -lbz2 -llzma -lz -lpng -luuid
LIBS += -lX11 -lXau -lXdmcp -lXext -lXfixes -lXv -lXinerama
LIBS += -lfontconfig -lfreetype
LIBS += -lavcodec -lavdevice -lavfilter -lavformat -lavresample -lavutil
LIBS += -lswresample -lswscale
LIBS += -lvpx -lwebp -lwebpmux

LIBS += $(if $(filter fftw,$(static_pkgs)), -lfftw3)
LIBS += $(if $(filter flac,$(static_pkgs)), -lFLAC)
LIBS += $(if $(filter giflib,$(static_pkgs)), -lgif)
LIBS += $(if $(filter lame,$(static_pkgs)), -lmp3lame)
LIBS += $(if $(filter libjpeg,$(static_pkgs)), -ljpeg)
LIBS += $(if $(filter openjpeg,$(static_pkgs)), -lopenjp2)
LIBS += $(if $(filter libogg,$(static_pkgs)), -logg)
LIBS += $(if $(filter libsndfile,$(static_pkgs)), -lsndfile)
LIBS += $(if $(filter libtheora,$(static_pkgs)), -ltheoradec -ltheoraenc)
LIBS += $(if $(filter libuuid,$(static_pkgs)), -luuid)
LIBS += $(if $(filter libvorbis,$(static_pkgs)), -lvorbis -lvorbisenc -lvorbisfile)
LIBS += $(if $(filter tiff,$(static_pkgs)), -ltiff)
LIBS += $(if $(filter twolame,$(static_pkgs)), -ltwolame)
LIBS += $(if $(filter x264,$(static_pkgs)), -lx264)
LIBS += $(if $(filter x265,$(static_pkgs)), -lx265)
LIBS += $(if $(filter libvpx,$(static_pkgs)), -lvpx)

ifneq ($(WANT_GL),no)
LIBS += -lGL -lGLU -lGLX
endif
ifneq ($(WANT_XFT),no)
LIBS += -lXft
endif
ifneq ($(WANT_ALSA),no)
LIBS += -lasound
endif
ifneq ($(WANT_OPENEXR),no)
LIBS += -lIex -lIexMath -lIlmImf -lIlmImfUtil
endif
ifneq ($(WANT_DV),no)
LIBS += -ldv
endif
ifneq ($(WANT_FIREWIRE),no)
LIBS += -lavc1394 -lraw1394 -liec61883 -lrom1394
endif
ifneq ($(WANT_DVB),no)
LIBS += -ldv
endif
ifneq ($(WANT_DL),no)
LIBS += -ldl
endif
ifneq ($(WANT_LIBZMPEG),no)
LIBS += -lmjpegutils
endif
ifneq ($(WANT_OPUS),no)
LIBS += -lopus
endif
ifneq ($(WANT_NUMA),no)
LIBS += -lnuma
endif
ifneq ($(WANT_LV2),no)
LIBS += -llilv-0 -lsratom-0 -lserd-0 -lsord-0 -lsuil-0
endif
endif

ifneq ($(WANT_COMMERCIAL),no)
CUTADS = $(OBJDIR)/cutads
CUTOBJ = $(OBJDIR)/cutads.o
CUTOBJS = $(CUTOBJ) \
	$(OBJDIR)/mediadb.o \
	$(OBJDIR)/filexml.o
CUTLIBS = \
	$(LIBZMPEG3)/$(OBJDIR)/libzmpeg3.a -lX11 \
	$(TDB)/$(OBJDIR)/db.a
endif

BDWRITE = $(OBJDIR)/bdwrite
BDWOBJS = $(OBJDIR)/bdwrite.o

ifneq ($(WANT_LV2),no)
LV2UI = $(OBJDIR)/lv2ui
LV2OBJS = \
	$(OBJDIR)/lv2ui.o \
	$(OBJDIR)/pluginlv2.o \
	$(OBJDIR)/pluginlv2ui.o \
	$(OBJDIR)/pluginlv2config.o \
	$(OBJDIR)/forkbase.o
endif

CFLAGS := \
	-I$(GUICAST) \
	-I$(LIBZMPEG3) \
	$(static_incs) \
	$(CFLAGS)

# Speed up linking with this linking sequence
ifeq ($(OBJDIR), alpha)

LDFLAGS1 = \
	--demangle=compaq -export-dynamic -L./ \
	-L$(GUICAST) -L/usr/X11R6/lib \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`lib -L/usr/local/lib \
	-L`expr /usr/lib/gcc-lib/alpha-redhat-linux/*` \
	-rpath `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -m elf64alpha \
	-L`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/ -dynamic-linker \
	/lib/ld-linux.so.2 `expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crt1.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crti.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtbegin.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/_main.o

LDFLAGS2 = -lcpml -lcxxstdma_rh60 -lcxxma_rh60 -lc -lots \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtend.o \
	`expr /usr/lib/compaq/cxx-*/alpha-linux/`bin/crtn.o --no-demangle \
	--warn-once $(LDFLAGS)
LINKER = ld -o $(OUTPUT)
CFLAGS += -DUSE_ALPHA

else

LDFLAGS1 = -Wl,-export-dynamic -g
LDFLAGS2 = $(LDFLAGS)
LINKER = $(CXX) -o $(OUTPUT)

endif

$(shell mkdir -p $(OBJDIR))
$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(LDFLAGS1) $(OBJS) $(THEME_DATA) $(DCRAW) $(LIBS) $(LDFLAGS2) > $(OBJDIR)/objs)

all:	$(OUTPUT) $(CUTADS) $(BDWRITE) $(LV2UI)

# Static linking is not possible because the plugins depend on symbols
# in the main executable.
# Also VFS only overrides the C library when dynamic linking is used.
$(OUTPUT): $(OBJS) $(THEME_DATA) $(DCRAW) $(LIBRARIES)
	$(LINKER) `cat $(OBJDIR)/objs`
	$(if $(findstring -g,$(CFLAGS)),objcopy --only-keep-debug $(OUTPUT) $(OUTPUT_G))
	$(if $(findstring -ggdb,$(CFLAGS)),,strip $(OUTPUT))
	ln -f -s ../bin/$(WANT_CIN) ci

ifneq ($(WANT_COMMERCIAL),no)
$(CUTADS):	$(CUTOBJS) $(CUTLIBS) $(LIBRARIES)
	@echo g++ -o $@ $(CUTOBJS)
	@g++ $(CFLAGS) -pthread -o $@ $(CUTOBJS) $(CUTLIBS) $(LIBS)
	$(if $(findstring -g,$(CFLAGS)),objcopy --only-keep-debug $(CUTADS) $(CUTADS).debuginfo)
	$(if $(findstring -ggdb,$(CFLAGS)),,strip $(CUTADS))

install-cutads:
	cp -av $(OBJDIR)/cutads $(BINDIR)/.

install:	install-cutads
endif

$(BDWRITE):	$(BDWOBJS) $(LIBRARIES)
	@echo g++ -o $@ $(BDWOBJS)
	@g++ $(CFLAGS) -pthread -o $@ $(BDWOBJS) $(LIBS)
	$(if $(findstring -g,$(CFLAGS)),objcopy --only-keep-debug $(BDWRITE) $(BDWRITE).debuginfo)
	$(if $(findstring -ggdb,$(CFLAGS)),,strip $(BDWRITE))

install-bdwrite:
	cp -av $(OBJDIR)/bdwrite $(BINDIR)/.
install:	install-bdwrite

ydiff:	ydiff.C $(LIBRARIES)
	@echo g++ -o $@ ydiff.C
	@g++ $(CFLAGS) ydiff.C -o $@ $(LIBS)

shdmp:	shdmp.C
	@g++ $(CFLAGS) shdmp.C -o $@
shudmp:	shudmp.C
	@g++ $(CFLAGS) shudmp.C -o $@ -lusb-1.0

clean:
	rm -rf $(OBJDIR)
	rm -f shuttle_keys.h


tags:
	ctags -R -h default --langmap=c:+.inc . ../guicast/ ../libzmpeg3 ../plugins ../thirdparty/ffmpeg-*


$(OBJDIR)/%.o:		%.C
	$(CXX) `cat $(OBJDIR)/c_flags` -DMSGQUAL=$* -c $< -o $@


$(OBJDIR)/fileexr.o:	fileexr.C
	$(CXX) `cat $(OBJDIR)/c_flags` -Wno-deprecated -DMSGQUAL=$* -c $< -o $@

$(OBJDIR)/sha1.o:	sha1.C sha1.h
	$(CXX) `cat $(OBJDIR)/c_flags` -O3 -c $< -o $@

#lv2
ifneq ($(WANT_LV2),no)
GTK2_INCS := `pkg-config --cflags gtk+-2.0`
GTK2_LIBS := `pkg-config --libs gtk+-2.0`

$(OBJDIR)/pluginlv2ui.o:	pluginlv2ui.C
$(OBJDIR)/lv2ui.o:	lv2ui.C
$(OBJDIR)/pluginlv2ui.o $(OBJDIR)/lv2ui.o:
	$(CXX) `cat $(OBJDIR)/c_flags` $(GTK2_INCS) -DMSGQUAL=$* -c $< -o $@

$(OBJDIR)/shuttle.o:	shuttle.C shuttle_keys.h
	$(CXX) `cat $(OBJDIR)/c_flags` -DMSGQUAL=$* -c $< -o $@
shuttle_keys.h: /usr/include/X11/keysymdef.h
	sed < /usr/include/X11/keysymdef.h > shuttle_keys.h -f shuttle.sed

$(OBJDIR)/lv2ui: $(LV2OBJS)
	@echo $(CXX) \`cat $(OBJDIR)/c_flags\` $^ -o $@
	@$(CXX) `cat $(OBJDIR)/c_flags` $^ -o $@ $(LIBS) $(GTK2_LIBS)
	$(if $(findstring -g,$(CFLAGS)),objcopy --only-keep-debug $(LV2UI) $(LV2UI).debuginfo)
	$(if $(findstring -ggdb,$(CFLAGS)),,strip $(LV2UI))

.PHONY:	lv2ui
lv2ui:	$(LV2UI)

install-lv2ui:
	cp -av $(LV2UI) $(BINDIR)/.

install:	install-lv2ui
endif

$(OBJDIR)/pluginlv2gui.o:	pluginlv2gui.C
	$(CXX) `cat $(OBJDIR)/c_flags` $(GTK2_INCS) -DMSGQUAL=$* -c $< -o $@

#dcraw
$(DCRAW): dcraw.C
	$(CXX) `cat $(OBJDIR)/c_flags` $(if $(findstring -ggdb,$(CFLAGS)),,-O4) -DNODEPS -DLOCALTIME \
		-Wno-misleading-indentation -Wno-sign-compare -Wno-narrowing dcraw.C -c -o $*.o

$(THEME_DATA):
	cd $(OBJDIR) && \
	$(GUICAST)/$(OBJDIR)/bootstrap theme_data.o $(TOPDIR)/picon/cinfinity/*.png

val-%:
	@echo $($(subst val-,,$@))

