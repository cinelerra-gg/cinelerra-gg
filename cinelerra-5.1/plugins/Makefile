export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config
default: all

ifeq ($(WANT_DVB), yes)
LIVEDVB = liveaudio livevideo
endif
ifneq ($(WANT_OPENCV), no)
want_var:=$(WANT_OPENCV)
include $(TOPDIR)/opencv_build
OPENCV_OBJS := findobj flowobj gaborobj moveobj stylizeobj puzzleobj
$(OPENCV_OBJS): opencv
endif

# burn must come before any other effecttv plugin
# colors must come before any plugin
# parametric must come before fourier plugins
# motion must come before perspective

DIRS = $(OPENCV_OBJS) \
	1080to480 \
	1080to540 \
	720to480 \
	aging \
	scaleratio \
	audioscope \
	bandslide \
	bandwipe \
	bluebanana \
	blur \
	brightness \
	burn \
	C41 \
	cdripper \
	chromakey \
	chromakeyhsv \
	color3way \
	colorbalance \
	compressor \
	crikey \
	crossfade \
	dcoffset \
	decimate \
	deinterlace \
	deinterlace-cv \
	delayaudio \
	delayvideo \
	denoise \
	denoisefft \
	denoiseseltempavg \
	denoisevideo \
	descratch \
	despike \
	diffkey \
	dissolve \
	dot \
	downsample \
	edge \
	echo \
	echocancel \
	fieldframe \
	flash \
	flip \
	framefield \
	freeverb \
	freezeframe \
	gain \
	gamma \
	gradient \
	graphic \
	histeq \
	histogram \
	histogram_bezier \
	holo \
	huesaturation \
	interpolate \
	interpolateaudio \
	interpolatevideo \
	invertaudio \
	invertvideo \
	irissquare \
	ivtc \
	lens \
	level \
	libeffecttv \
	linearblur \
	$(LIVEDVB) \
	loopaudio \
	loopvideo \
	motion \
	motion51 \
	motion-cv \
	motion-hv \
	motion2point \
	motionblur \
	normalize \
	oilpainting \
	overlay \
	overlayaudio \
	parametric \
	perspective \
	photoscale \
	pitch \
	polar \
	radialblur \
	reframe \
	reframert \
	removegaps \
	reroute \
	resample \
	resamplert \
	reverb \
	reverseaudio \
	reversevideo \
	rgb601 \
	rgbshift \
	rotate \
	rumbler \
	scale \
	sketcher \
	shapewipe \
	sharpen \
	shiftinterlace \
	slide \
	spectrogram \
	spherecam \
	svg \
	swapchannels \
	swapframes \
	synthesizer \
	threshold \
	timeavg \
	timefront \
	timestretch \
	timestretchrt \
	titler \
	translate \
	unsharp \
	videoscope \
	wave \
	whirl \
	wipe \
	yuv \
	yuv411 \
	yuvshift \
	zoom \
	zoomblur \
	theme_blond \
	theme_blond_cv \
	theme_blue \
	theme_blue_dot \
	theme_bright \
	theme_hulk \
	theme_neophyte \
	theme_pinklady \
	theme_suv \
	theme_unflat \

# too costly
#	findobject \
#	greycstoration \
# not finished
#	denoisemjpeg \
# duplicate
#	vocoder \

PLUGIN_DIR = $(BINDIR)/plugins
DATA = $(PLUGIN_DIR)/fonts $(PLUGIN_DIR)/shapes
LADSPA_DIR = $(BINDIR)/ladspa

all:	$(PLUGIN_DIR) $(DIRS) $(DATA) $(LADSPA_DIR)

$(PLUGIN_DIR):
	mkdir -p $@

$(DATA):	$(PLUGIN_DIR)
	cp -a $(notdir $@) $(PLUGIN_DIR)/.

$(LADSPA_DIR):
	mkdir -p $@
	if [ -d $(THIRDPARTY)/ladspa-*/usr/local/lib*/ladspa ]; then \
		cp -a $(THIRDPARTY)/ladspa-*/usr/local/lib*/ladspa/. $@/. ; \
	fi

# parallel compilation of all directories
.PHONY:	$(DIRS)
$(DIRS):
	$(MAKE) -C $@

clean:
	rm -rf $(foreach d,$(DIRS),$(d)/$(OBJDIR))
	rm -rf $(PLUGIN_DIR) $(LADSPA_DIR)

install:

# dependencies for parallel build
aging:		libeffecttv
burn:		libeffecttv
dot:		libeffecttv
holo:		libeffecttv
motion2point:	downsample motion
motion:		downsample
timestretchrt:	timestretch

