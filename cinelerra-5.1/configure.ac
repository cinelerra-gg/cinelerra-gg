
AC_PREREQ([2.69])
AC_INIT([cinelerra], [5.1], [mail@lists.cinelerra-gg.org])
AM_INIT_AUTOMAKE([foreign])

AM_PROG_AS
AC_LANG_CPLUSPLUS
AC_LANG_C
AC_PROG_CXX

CFG_CFLAGS+=" -fno-omit-frame-pointer -fno-math-errno -fno-signed-zeros"
CFG_CFLAGS+=" -pthread -Wall"
# misguided pedantic warnings
#  this is extra work, not a gain...
CFG_CFLAGS+=" -Wno-unknown-warning"
CFG_CFLAGS+=" -Wno-attributes"
CFG_CFLAGS+=" -Wno-unused-result"
CFG_CFLAGS+=" -Wno-stringop-overflow"
CFG_CFLAGS+=" -Wno-format-truncation"
CFG_CFLAGS+=" -Wno-format-overflow"
CFG_CFLAGS+=" -Wno-parentheses"
CFG_CFLAGS+=" -Wno-stringop-truncation"
CFG_CFLAGS+=" -D__STDC_CONSTANT_MACROS"
CFG_CFLAGS+=" -D__STDC_LIMIT_MACROS"
CFG_CFLAGS+=" -DPNG_SKIP_SETJMP_CHECK=1"
CFG_CFLAGS+=" -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"
CFG_CFLAGS+=" -I/usr/include -I/usr/local/include"
CFG_CFLAGS+=" -I/usr/include/freetype2 -I/usr/local/include/freetype2"
CFG_CFLAGS+=" -I/usr/include/uuid -I/usr/local/include/uuid"
CFG_CFLAGS+=" -I/usr/include/mjpegtools -I/usr/local/include/mjpegtools"

CFG_LDFLAGS+=" -z noexecstack"

AC_DEFUN([CHECK_WITH], [
AC_ARG_WITH([$1],
  AC_HELP_STRING([--with-[$1]], [$2 ($4)]),
  [WANT_$3=$withval], [WANT_$3='$4'])
AC_SUBST([WANT_$3])
])

CHECK_WITH([jobs],[parallel build jobs],[JOBS],[auto])
CHECK_WITH([exec-name],[binary executable name],[CIN],[cin])
CHECK_WITH([single-user],[to install cin in bin],[CINBIN_BUILD],[no])
CHECK_WITH([ladspa-build],[build ladspa library],[LADSPA_BUILD],[yes])
CHECK_WITH([lv2],[lv2 library support],[LV2],[yes])
CHECK_WITH([cinlib],[cinelerra library path],[CINLIB_DIR],[auto])
CHECK_WITH([cindat],[cinelerra share path],[CINDAT_DIR],[auto])
CHECK_WITH([plugin-dir],[plugin install dir],[PLUGIN_DIR],[auto])
CHECK_WITH([ladspa-dir],[ladspa install dir],[LADSPA_DIR],[auto])
CHECK_WITH([config-dir],[.bcast config dir],[CONFIG_DIR],[$$HOME/.bcast5])
CHECK_WITH([browser],[cin_browser path],[CIN_BROWSER],[firefox])
CHECK_WITH([git-ffmpeg],[git ffmpeg using url],[GIT_FFMPEG],[no])
CHECK_WITH([noelision],[use noelision/libpthread],[NOELISION],[auto])
CHECK_WITH([booby],[window lock trace booby trap],[BOOBY],[no])
CHECK_WITH([libzmpeg],[build libzmpeg],[LIBZMPEG],[yes])
CHECK_WITH([commercial],[enable commercial capture],[COMMERCIAL],[yes])
CHECK_WITH([thirdparty],[use thirdparty build],[CIN_3RDPARTY],[yes])
CHECK_WITH([shuttle],[shuttle device],[SHUTTLE],[yes])
CHECK_WITH([vaapi],[video acceleration api],[VAAPI],[yes])
CHECK_WITH([vdpau],[video decode+presentation api for unix],[VDPAU],[yes])

if test "x$WANT_LV2" != "xno"; then
  GTK2_LIBS=`pkg-config --libs gtk+-2.0`
  AC_CHECK_LIB([m], [gtk_init], [HAVE_gtk=yes], [HAVE_gtk=no], [$GTK2_LIBS])
  test "x$HAVE_gtk" != "xno" && test -z "`pkg-config --cflags gtk+-2.0`" && HAVE_gtk=no
  test "x$HAVE_gtk" != "xno" && test -z "`pkg-config --libs gtk+-2.0`" && HAVE_gtk=no
  if test "x$HAVE_gtk" = "xno"; then
    AC_MSG_ERROR([LV2 requires gtk+-2.0 support.])
  fi
fi

if test "x$WANT_COMMERCIAL" = "xyes" -a "x$WANT_LIBZMPEG" != "xyes" ; then
  AC_MSG_ERROR([commercial requires libzmpeg support.])
fi

if test "x$WANT_CINBIN_BUILD" = "xyes"; then
  WANT_LOCALE_DIR='$$CIN_LIB/locale'
  test "x$WANT_CINDAT_DIR" = "xauto" && WANT_CINDAT_DIR='$$CIN_PATH'
  test "x$WANT_CINLIB_DIR" = "xauto" && WANT_CINLIB_DIR='$$CIN_PATH'
  test "x$WANT_PLUGIN_DIR" = "xauto" && WANT_PLUGIN_DIR='$$CIN_LIB/plugins'
  test "x$WANT_LADSPA_DIR" = "xauto" && WANT_LADSPA_DIR='$$CIN_LIB/ladspa'
else
  lcldir=`test "x$prefix" = "xNONE" && prefix=$ac_default_prefix; \
    test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"; \
    eval "dir=\"$localedir\""; eval "dir=\"$dir\""; eval "dir=\"$dir\""; \
    echo "$dir"`
  usrdat=`test "x$prefix" = "xNONE" && prefix=$ac_default_prefix; \
    test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"; \
    eval "dir=\"$datadir\""; eval "dir=\"$dir\""; eval "dir=\"$dir\""; \
    echo "$dir"`
  usrlib=`test "x$prefix" = "xNONE" && prefix="$ac_default_prefix"; \
    test "x$exec_prefix" = "xNONE" && exec_prefix="${prefix}"; \
    eval "dir=\"$libdir\""; eval "dir=\"$dir\""; eval "dir=\"$dir\""; \
    echo "$dir"`
  test "x$usrlib" = "x/lib" -o "x$usrlib" = "x/usr/lib" -o \
       "x$usrlib" = "x/usr/local/lib" -a -e "/usr/lib64" && \
  test ! -e "/usr/lib" -o `ls -1 /usr/lib64 | wc -l` -gt `ls -1 /usr/lib | wc -l` && \
    usrlib+="64"
  WANT_LOCALE_DIR="$lcldir"
  test "x$WANT_CINDAT_DIR" = "xauto" && WANT_CINDAT_DIR="$usrdat/$WANT_CIN"
  test "x$WANT_CINLIB_DIR" = "xauto" && WANT_CINLIB_DIR="$usrlib/$WANT_CIN"
  test "x$WANT_PLUGIN_DIR" = "xauto" && WANT_PLUGIN_DIR="$WANT_CINLIB_DIR/plugins"
  test "x$WANT_LADSPA_DIR" = "xauto" && WANT_LADSPA_DIR="$WANT_CINLIB_DIR/ladspa"
fi

CIN_INSTALLS=""
CIN_UNINSTALLS=""
if test "x$WANT_LIBZMPEG" = "xyes"; then
  CIN_INSTALLS+=" zmp_install"
  CIN_UNINSTALLS+=" zmp_uninstall"
fi
if test "x$WANT_COMMERCIAL" = "xyes"; then
  CIN_INSTALLS+=" com_install"
  CIN_UNINSTALLS+=" com_uninstall"
fi
if test "x$WANT_LADSPA_BUILD" = "xyes"; then
  CIN_INSTALLS+=" lad_install "
  CIN_UNINSTALLS+=" lad_uninstall "
fi
if test "x$WANT_LV2" = "xyes"; then
  CIN_INSTALLS+=" lv2_install "
  CIN_UNINSTALLS+=" lv2_uninstall "
fi
AC_SUBST(CIN_INSTALLS)
AC_SUBST(CIN_UNINSTALLS)

if test "x$WANT_CINBIN_BUILD" = "xyes"; then
  MAK_INSTALLS="dvl_install"
  MAK_UNINSTALLS="dvl_uninstall"
else
  MAK_INSTALLS=" sys_install"
  MAK_UNINSTALLS="sys_uninstall"
fi
AC_SUBST(MAK_INSTALLS)
AC_SUBST(MAK_UNINSTALLS)

if test "x$WANT_JOBS" = "xauto"; then
  CPUS=`grep -c "^proc" /proc/cpuinfo`
  WANT_JOBS=`expr $CPUS + $CPUS / 2 + 2`
fi

AC_DEFUN([PKG_DEF],[
STATIC_PKGS+=" [$1]"
ver_$1="$2"
lib_$1="$3"
inc_$1="$4"
])

AC_DEFUN([PKG_3RD],[
AC_ARG_ENABLE([$1],
  AC_HELP_STRING([--enable-$1],[build $1 ($2)]),
  [PKG_$1=$enableval], [PKG_$1=$2])
PKG_DEF([$1],[$3],[$4],[$5])
])

# thirdparty builds

PKG_3RD([a52dec],[yes],
  [a52dec-0.7.4],
  [ liba52/.libs/liba52.a ],
  [ include ])

PKG_3RD([djbfft],[yes],
  [djbfft-0.76],
  [ libdjbfft.a ],
  [ . ])

PKG_3RD([audiofile],[no],
  [audiofile-0.3.6],
  [ libaudiofile/.libs/libaudiofile.a \
    libaudiofile/modules/.libs/libmodules.a \
    libaudiofile/alac/.libs/libalac.a ],
  [ libaudiofile ])

PKG_3RD([encore],[no],
  [encore-5.0],
  [ \$(OBJDIR)/libencore.a ],
  [ . ])

PKG_3RD([esound],[no],
  [esound-0.2.41],
  [ .libs/libesd.a \
    .libs/libesddsp.a ],
  [ . ])

PKG_3RD([ffmpeg],[yes],
  [ffmpeg-4.1],
  [ libavutil/libavutil.a \
    libavcodec/libavcodec.a \
    libpostproc/libpostproc.a \
    libavdevice/libavdevice.a \
    libavformat/libavformat.a \
    libswscale/libswscale.a \
    libavfilter/libavfilter.a \
    libswresample/libswresample.a ],
  [ . ])

if test "x$WANT_GIT_FFMPEG" != "xno" ; then
  ver_ffmpeg="ffmpeg.git"
fi

PKG_3RD([fftw],[auto],
  [fftw-3.3.8],
  [ .libs/libfftw3.a \
    libbench2/libbench2.a \
    rdft/scalar/r2cb/.libs/librdft_scalar_r2cb.a \
    rdft/scalar/r2r/.libs/librdft_scalar_r2r.a \
    rdft/scalar/.libs/librdft_scalar.a \
    rdft/scalar/r2cf/.libs/librdft_scalar_r2cf.a \
    rdft/.libs/librdft.a \
    api/.libs/libapi.a \
    reodft/.libs/libreodft.a \
    dft/scalar/codelets/.libs/libdft_scalar_codelets.a \
    dft/scalar/.libs/libdft_scalar.a \
    dft/.libs/libdft.a \
    kernel/.libs/libkernel.a \
    simd-support/.libs/libsimd_support.a ],
  [ api ])

PKG_3RD([flac],[auto],
  [flac-1.3.2],
  [ src/libFLAC/.libs/libFLAC.a \
    src/libFLAC++/.libs/libFLAC++.a \
    src/share/replaygain_analysis/.libs/libreplaygain_analysis.a \
    src/share/replaygain_synthesis/.libs/libreplaygain_synthesis.a \
    src/share/grabbag/.libs/libgrabbag.a \
    src/share/utf8/.libs/libutf8.a \
    src/test_libs_common/.libs/libtest_libs_common.a ],
  [ include ])

PKG_3RD([giflib],[yes],
  [giflib-5.1.6],
  [ libgif.a ],
  [ . ])

PKG_3RD([ilmbase],[auto],
  [ilmbase-2.2.1],
  [ Iex/.libs/libIex.a \
    IexMath/.libs/libIexMath.a \
    Half/.libs/libHalf.a \
    Imath/.libs/libImath.a \
    IlmThread/.libs/libIlmThread.a ],
  [ Iex Half Imath config IlmThread ])

PKG_DEF([ladspa], [ladspa-0.4.17], [], [])

PKG_3RD([lame],[auto],
  [lame-3.100],
  [ libmp3lame/.libs/libmp3lame.a \
    mpglib/.libs/libmpgdecoder.a ],
  [ include ])

PKG_3RD([libavc1394],[auto],
  [libavc1394-0.5.4],
  [ libavc1394/.libs/libavc1394.a \
    librom1394/.libs/librom1394.a \
    common/.libs/libraw1394util.a ],
  [ . ])

PKG_3RD([libraw1394],[auto],
  [libraw1394-2.1.2],
  [ src/.libs/libraw1394.a ],
  [ . ])

PKG_3RD([libiec61883],[auto],
  [libiec61883-1.2.0],
  [ src/.libs/libiec61883.a ],
  [ src ])

PKG_3RD([libdv],[auto],
  [libdv-1.0.0],
  [ libdv/.libs/libdv.a ],
  [ . ])

PKG_3RD([libjpeg],[auto],
  [libjpeg-turbo-1.5.1],
  [ .libs/libjpeg.a \
    .libs/libturbojpeg.a \
    simd/.libs/libsimd.a ],
  [ . ])

PKG_3RD([opus],[auto],
  [opus-1.3],
  [ .libs/libopus.a ],
  [ include ])

PKG_3RD([openjpeg],[auto],
  [openjpeg-2.3.0],
  [ bin/libopenjp2.a ],
  [ src/lib/openjp2 ])

PKG_3RD([libogg],[auto],
  [libogg-1.3.3],
  [ src/.libs/libogg.a ],
  [ include ])

PKG_3RD([libsndfile],[auto],
  [libsndfile-1.0.28],
  [ src/.libs/libsndfile.a \
    src/.libs/libcommon.a \
    src/G72x/.libs/libg72x.a \
    src/GSM610/.libs/libgsm.a ],
  [ . ])

PKG_3RD([libtheora],[auto],
  [libtheora-1.1.1],
  [ lib/.libs/libtheora.a \
    lib/.libs/libtheoradec.a \
    lib/.libs/libtheoraenc.a ],
  [ . ])

PKG_3RD([libuuid],[yes],
  [libuuid-1.0.3],
  [ .libs/libuuid.a ],
  [ . ])

PKG_3RD([libvorbis],[auto],
  [libvorbis-1.3.6],
  [ lib/.libs/libvorbis.a \
    lib/.libs/libvorbisenc.a \
    lib/.libs/libvorbisfile.a ],
  [ include . ])

PKG_3RD([mjpegtools],[yes],
  [mjpegtools-2.1.0],
  [ utils/mmxsse/.libs/libmmxsse.a \
    utils/.libs/libmjpegutils.a \
    lavtools/.libs/liblavfile.a \
    lavtools/.libs/liblavjpeg.a \
    mplex/.libs/libmplex2.a \
    yuvfilters/.libs/libyuvfilters.a \
    mpeg2enc/.libs/libmpeg2encpp.a ],
  [ .  lavtools utils ])

PKG_3RD([openexr],[auto],
  [openexr-2.2.1],
  [ IlmImf/.libs/libIlmImf.a \
    IlmImfUtil/.libs/libIlmImfUtil.a ],
  [ IlmImf config ])

#PKG_3RD([festival],[no],
#  [festival],
#  [],
#  [])
#PKG_3RD(speech_tools,[no],
#  [speech_tools],
#  [],
#  [])
#
PKG_3RD([tiff],[auto],
  [tiff-4.0.10],
  [ libtiff/.libs/libtiff.a \
    libtiff/.libs/libtiffxx.a \
    port/.libs/libport.a ],[
   . ])

PKG_3RD([twolame],[auto],
  [twolame-0.3.13],
  [ libtwolame/.libs/libtwolame.a ],
  [  libtwolame ])

PKG_3RD([x264],[auto],
  [x264-snapshot-20190117-2245-stable],
  [ libx264.a ],
  [ . ])

PKG_3RD([x265],[auto],
  [x265_3.0],
  [ libx265.a ],
  [ . source ])

PKG_3RD([libvpx],[auto],
  [libvpx-1.8.0],
  [ libvpx.a ],
  [ . ])

PKG_3RD([lv2],[auto],
  [lv2-1.16.0],
  [ ],
  [ usr/local/include usr/local/lib64/lv2 usr/local/lib/lv2 ])

PKG_3RD([sratom],[auto],
  [sratom-0.6.2],
  [ usr/local/lib/libsratom-0.a ],
  [ usr/local/include ])

PKG_3RD([serd],[auto],
  [serd-0.30.0],
  [ usr/local/lib/libserd-0.a ],
  [ usr/local/include ])

PKG_3RD([sord],[auto],
  [sord-0.16.2],
  [ usr/local/lib/libsord-0.a ],
  [ usr/local/include ])

PKG_3RD([lilv],[auto],
  [lilv-0.24.4],
  [ usr/local/lib/liblilv-0.a ],
  [ usr/local/include ])

PKG_3RD([suil],[auto],
  [suil-0.10.2],
  [ usr/local/lib/libsuil-0.a ],
  [ usr/local/include ])

PKG_3RD([libaom],[auto],
  [libaom-v1.0.0],
  [ usr/local/lib*/libaom*.a ],
  [ usr/local/include ])

PKG_3RD([libwebp],[auto],
  [libwebp-1.0.2],
  [ usr/local/lib*/libwebp*.a ],
  [ usr/local/include ])

AC_SUBST(STATIC_PKGS)

AC_DEFUN([CHECK_ENABLE], [
AC_ARG_ENABLE([$1],
  AC_HELP_STRING([--enable-[$1]], [$3 ([$4])]),
  [WANT_$2=$enableval], [WANT_$2=[$4]])
])

CHECK_ENABLE([static-build], [STATIC_BUILD], [build static], [auto])
CHECK_ENABLE([x264_hidepth], [X264_HIDEPTH], [build x264 10bit], [no])
CHECK_ENABLE([x265_hidepth], [X265_HIDEPTH], [build x265 10bit], [no])

test "x$WANT_STATIC_BUILD" = "xauto" && WANT_STATIC_BUILD=$WANT_CINBIN_BUILD

AC_DEFUN([CHECK_PROG], [
 AC_CHECK_PROG([PROG_$1], [$2], [yes],[no])
])
AC_DEFUN([REQUIRE_PROG], [
 CHECK_PROG([$1],[$2])
 if test "x$PROG_$1" = "xno"; then
   AC_MSG_WARN([$2 is missing and required.])
   FATAL_ERROR=1
 fi
])

## arch dep tests
ARCH=[`uname -m`]
I86=[`expr "x$ARCH" : 'xi[346]86.*'`]
X86=[`expr "x$ARCH" : 'x..._64*'`]

if test "x$I86$X86" != "x00" ; then
  # Checks for ix86 programs.
  REQUIRE_PROG(NASM, [nasm])
  # libx264 nasm fix
  AC_MSG_CHECKING([nasm x264 compatible])
  echo "vmovdqa32 [[eax]]{k1}{z}, zmm0" > conftest.asm
  nasm conftest.asm -o conftest.o > /dev/null 2>&1
  if test $? != 0 ; then
   AC_MSG_RESULT([no])
   AC_MSG_WARN([libx264 built without assembly code])
   X264_CFG_PARAMS="$X264_CFG_PARAMS --disable-asm"
  else
   AC_MSG_RESULT([yes])
  fi
  rm -f conftest.asm conftest.o
  REQUIRE_PROG(YASM, [yasm])
fi
## end arch dep tests

REQUIRE_PROG(OBJCOPY, [objcopy])
if test "x$FATAL_ERROR" != "x"; then
 AC_MSG_ERROR("fatal eror.")
fi
CHECK_PROG(PACTL, [pactl])
WANT_PACTL=$PROG_PACTL

AC_CHECK_DECL([X_HAVE_UTF8_STRING],,[no_utf=yes],[#include <X11/Xlib.h>])
if test "$no_utf" = "yes"; then
  AC_MSG_ERROR([Cinelerra requires utf8 support in X Windows.])
fi

# Checks for headers/libraries.

AC_DEFUN([CHECK_HEADERS], [
if test "x$HAVE_$1" != "xno"; then
 AC_CHECK_HEADER(m4_apply([m4_car],m4_split([$3])), [HAVE_$1=yes], [HAVE_$1=no],
  [m4_foreach([i], m4_split([$3]), [[#include "]i["
]])])
 AC_SUBST([HAVE_$1])
 AC_MSG_CHECKING([$2])
 AC_MSG_RESULT([$HAVE_$1])
 if test "x$HAVE_$1" = "xno"; then
  if test "x$PKG_$1" = "x"; then RESULT=no; fi
 fi
fi
])

AC_DEFUN([CHECK_LIB], [
if test "x$HAVE_$1" != "xno"; then
 if test "x$PKG_$1" = "x" -o "x$WANT_STATIC_BUILD" = "xno" -a "x$PKG_$1" = "xauto"; then
  AC_CHECK_LIB([$2], [$3], [HAVE_$1=yes], [HAVE_$1=no], [$4])
  AC_SUBST([HAVE_$1])
  if test "x$WANT_$1" != "xno"; then
   if test "x$HAVE_$1" = "xyes"; then
    if test "x$PKG_$1" = "x"; then
     SYSTEM_LIBS+=" -l[$2]"
     for lib in [$4]; do SYSTEM_LIBS+=" $lib"; done
    else
     SHARED_$1+=" -l[$2]"
     for lib in [$4]; do SHARED_LIBS+=" $lib"; done
    fi
   elif test "x$PKG_$1" = "x"; then
    RESULT=no
   fi
  fi
 fi
fi
])

CHECK_LIB([X11], [X11], [XOpenDisplay])
CHECK_HEADERS([X11], [X11 headers], [X11/X.h X11/Xlib.h X11/Xutil.h X11/cursorfont.h])
CHECK_HEADERS([X11], [X11 headers], [X11/keysym.h X11/Sunkeysym.h X11/Xatom.h X11/XF86keysym.h])
CHECK_LIB([XEXT], [Xext], [XShmQueryExtension])
CHECK_HEADERS([XEXT], [Xlib XShm extention], [X11/Xlib.h X11/extensions/XShm.h X11/extensions/shape.h])
CHECK_LIB([XINERAMA], [Xinerama], [XineramaQueryExtension])
CHECK_HEADERS([XINERAMA], [Xinerama headers], [X11/extensions/Xinerama.h])
CHECK_LIB([XV], [Xv], [XvQueryExtension])
CHECK_HEADERS([XV], [Xlib Xv extention], [X11/Xlib.h X11/extensions/Xvlib.h])
CHECK_LIB([XFIXES], [Xfixes], [XFixesQueryVersion])
CHECK_LIB([BZ2], [bz2], [BZ2_bzDecompress])
CHECK_LIB([FONTCONFIG], [fontconfig], [FcInit])
CHECK_LIB([FREETYPE], [freetype], [FT_Init_FreeType])
CHECK_LIB([LZMA], [lzma], [lzma_version_number])
CHECK_LIB([PNG], [png], [png_read_png])
CHECK_LIB([PTHREAD], [pthread], [pthread_create])
CHECK_LIB([Z], [z], [gzopen])
CHECK_LIB([fftw], [fftw3], [fftw_execute])
CHECK_HEADERS([fftw], [fftw headers], [fftw3.h])
CHECK_LIB([libuuid], [uuid], [uuid_clear])
CHECK_HEADERS([libuuid], [uuid headers], [uuid/uuid.h])
CHECK_HEADERS([LINUX_CDROM], [linux cdrom headers], [linux/cdrom.h linux/futex.h])
CHECK_HEADERS([LINUX_SYS], [linux sys], [sys/ioctl.h sys/stat.h sys/types.h sys/time.h sys/syscall.h])
CHECK_LIB([flac], [FLAC], [FLAC__stream_decoder_new])
CHECK_HEADERS([flac], [FLAC headers], [FLAC/stream_decoder.h])
CHECK_LIB([lame], [mp3lame], [lame_init])
CHECK_HEADERS([lame], [lame headers], [lame/lame.h])
CHECK_LIB([libjpeg], [jpeg], [jpeg_start_decompress])
CHECK_HEADERS([libjpeg], [jpeg headers], [stdio.h jpeglib.h])
CHECK_LIB([libogg], [ogg], [ogg_stream_init])
CHECK_HEADERS([libogg], [ogg headers], [ogg/ogg.h])
CHECK_LIB([openjpeg], [openjp2], [opj_version])
CHECK_HEADERS([openjpeg], [openjpeg headers], [openjpeg.h])
CHECK_LIB([libsndfile], [sndfile], [sf_open])
CHECK_HEADERS([libsndfile], [sndfile headers], [sndfile.h])
CHECK_LIB([ilmbase], [IlmImf], [ImfOpenInputFile])
CHECK_HEADERS([ilmbase], [IlmImf headers], [OpenEXR/ImfCRgbaFile.h])
CHECK_LIB([libtheora], [theora], [theora_info_init], [-ltheoraenc -ltheoradec -logg])
CHECK_HEADERS([libtheora], [threora headers], [theora/theoraenc.h])
CHECK_LIB([libvorbis], [vorbisenc], [vorbis_encode_init], [-lvorbis -lvorbisfile -logg])
CHECK_HEADERS([libvorbis], [vorbis encoders headers], [vorbis/vorbisenc.h])
CHECK_LIB([libvorbis], [vorbisfile], [ov_open])
CHECK_HEADERS([libvorbis], [vorbis file headers], [vorbis/vorbisfile.h])
CHECK_LIB([libvpx], [vpx], [vpx_codec_decode])
CHECK_HEADERS([libvpx], [vpx headers], [vpx/vpx_decoder.h])
CHECK_LIB([mjpegtools], [mjpegutils], [mjpeg_info])
CHECK_HEADERS([mjpegtools], [mjpegtools headers], [mjpegtools/mjpeg_types.h])
CHECK_LIB([tiff], [tiff], [TIFFOpen])
CHECK_HEADERS([tiff], [tiff headers], [tiff.h tiffio.h])
CHECK_LIB([twolame], [twolame], [twolame_encode_buffer_float32_interleaved])
CHECK_HEADERS([twolame], [twolame headers], [twolame.h])
CHECK_LIB([x264], [x264], [x264_encoder_encode])
CHECK_HEADERS([x264], [x264 headers], [stdint.h x264.h])
CHECK_LIB([x265], [x265], [x265_encoder_encode])
CHECK_HEADERS([x265], [x265 headers], [x265.h])
CHECK_LIB([opus], [opus], [opus_multistream_decoder_create])
CHECK_HEADERS([opus], [libopus headers], [opus/opus_multistream.h])
CHECK_LIB([libaom], [aom], [aom_codec_version])
CHECK_HEADERS([libaom], [libaom headers], [aom/aom.h])
CHECK_LIB([libwebp], [webp], [WebPGetEncoderVersion])
CHECK_HEADERS([libwebp], [libwebp headers], [webp/encode.h])
CHECK_LIB([a52dec], [a52], [a52_init])
CHECK_HEADERS([a52dec], [a52 headers], [a52.h])
CHECK_LIB([encore], [encore], [encore])
CHECK_HEADERS([encore], [encore headers], [encore.h])
CHECK_LIB([giflib], [gif], [DGifOpen])
CHECK_HEADERS([giflib], [gif lib headers], [gif_lib.h])
CHECK_LIB([jbig], [jbig], [jbg_dec_init])
CHECK_LIB([VDPAU], [vdpau], [vdp_device_create_x11])
CHECK_LIB([VAAPI], [va], [vaInitialize])
if test "x$WANT_VAAPI" != "xno" -a "x$HAVE_VAAPI" = "xyes"; then
  CHECK_HEADERS([vaapi_x11], [va x11 headers], [va/va_x11.h])
  CHECK_LIB([vaapi_x11], [va-x11], [vaGetDisplay])
  CHECK_HEADERS([vaapi_drm], [va drm headers], [va/va_drm.h])
  CHECK_LIB([vaapi_drm], [va-drm], [vaGetDisplayDRM])
fi

#if test "x$HAVE_mjpegtools" = "xyes"; then
#CFG_CFLAGS+=" -I/usr/include/mjpegtools -I/usr/local/include/mjpegtools"
#fi

AC_DEFUN([CHECK_WANT], [
RESULT=yes
# WANT_$1
if test "x$WANT_$1" != "xno" ; then
AC_ARG_WITH(m4_tolower([$1]),
 AC_HELP_STRING([m4_join([-],[--with],m4_tolower([$1]))], [$3] (auto)),
  [WANT_$1=$withval],[WANT_$1=$2])
 m4_quote($4)
 if test "x$RESULT" = "xno" -a "x$PKG_$1" = "x"; then
  if test "x$WANT_$1" = "xyes" ; then
    AC_MSG_ERROR([required for $1 support.])
  fi
  echo "=== want $1 Failed."
  WANT_$1=no
 elif test "x$WANT_$1" = "xauto" ; then
  WANT_$1=yes
 fi
fi
CFG_WANTS+=" $1"
AC_SUBST([WANT_$1])
])

CHECK_WANT([GL], [auto], [use opengl], [
 CHECK_LIB([GL], [GL], [glUseProgram])
 CHECK_LIB([GL], [GLU], [gluOrtho2D])
 CHECK_HEADERS([GL], [opengl headers], [GL/gl.h GL/glext.h GL/glu.h GL/glx.h])])

CHECK_WANT([OSS], [auto], [use OSS audio], [
 CHECK_HEADERS([OSS], [oss headers], [sys/soundcard.h])])

CHECK_WANT([XFT], [auto], [use libXft], [
 CHECK_LIB([XFT], [Xft], [XftInit])
 CHECK_HEADERS([XFT], [Xft/freetype headers], [X11/Xlib.h X11/Xft/Xft.h])])

CHECK_WANT([ALSA], [auto], [use libasound/alsa], [
 CHECK_LIB([ALSA], [asound], [snd_pcm_open])
 CHECK_HEADERS([ALSA], [asound headers], [alsa/asoundlib.h])])

CHECK_WANT([FIREWIRE], [auto], [use firewire], [
 CHECK_LIB([libavc1394], [avc1394], [avc1394_init_target])
 CHECK_HEADERS([libavc1394], [libavc1394 headers], [libavc1394/avc1394.h])
 CHECK_LIB([libavc1394], [rom1394], [rom1394_get_bus_id])
 CHECK_HEADERS([libavc1394], [librom1394 headers], [libavc1394/rom1394.h])
 CHECK_LIB([libiec61883], [iec61883], [iec61883_mpeg2_recv_init])
 CHECK_HEADERS([libiec61883], [libiec61883 headers], [libiec61883/iec61883.h])
 CHECK_LIB([libraw1394], [raw1394], [raw1394_iso_recv_init])
 CHECK_HEADERS([libraw1394], [libraw1394 headers], [libraw1394/raw1394.h])])

CHECK_WANT([DV], [auto], [use dv], [
 CHECK_LIB([libdv], [dv], [dv_init])
 CHECK_HEADERS([libdv], [libdv headers], [libdv/dv.h])])

CHECK_WANT([DVB], [auto], [use dvb], [
 CHECK_HEADERS([DVB], [dvb device headers], [linux/dvb/dmx.h linux/dvb/frontend.h])])

CHECK_WANT([VIDEO4LINUX2], [auto], [use v4l2], [
 CHECK_HEADERS([VIDEO4LINUX2], [v4l2 system headers], [linux/kernel.h linux/types.h linux/videodev2.h])])

CHECK_WANT([XXF86VM], [auto], [use xf86vmode], [
 CHECK_HEADERS([XXF86VM], [XF86VM headers], [X11/Xlib.h X11/extensions/xf86vmode.h])])

CHECK_WANT([ESOUND], [no], [use esd], [
 CHECK_LIB([esound], [esd], [esd_open_sound])
 CHECK_HEADERS([esound], [esound headers], [esd.h])
 CHECK_LIB([audiofile], [audiofile], [afOpenFile])
 CHECK_HEADERS([audiofile], [audiofile headers], [audiofile.h])])

CHECK_WANT([SHUTTLE], [yes], [shuttle dev support], [
 CHECK_HEADERS([keysyms], [x11 keysym defs], [X11/keysymdef.h])])
if test "x$WANT_SHUTTLE" = "xno"; then
 WANT_SHUTTLE_USB="no"
fi
CHECK_WANT([SHUTTLE_USB], [yes], [use libusb-1.0], [
 CHECK_LIB([libusbx], [usb-1.0], [libusb_init])
 CHECK_HEADERS([libusbx], [libusb headers], [libusb-1.0/libusb.h])])

CHECK_WANT([LV2], [auto], [use lv2], [
 saved_CFLAGS="$CFLAGS"
 CHECK_LIB([lilv], [lilv-0], [lilv_world_new])
 CHECK_LIB([sord], [sord-0], [sord_world_new])
 CHECK_LIB([serd], [serd-0], [serd_reader_new])
 CHECK_LIB([sratom], [sratom-0], [sratom_new])
 CFLAGS="-I/usr/include/lilv-0 -I/usr/local/include/lilv-0"
 CHECK_HEADERS([lv2], [lilv headers], [lilv/lilv.h])
 CHECK_LIB([suil], [suil-0], [suil_instance_new])
 CFLAGS="-I/usr/include/suil-0 -I/usr/local/include/suil-0"
 CHECK_HEADERS([lv2], [suil headers], [suil/suil.h])
 CFLAGS="$saved_CFLAGS"])

CHECK_WANT([DL], [auto], [system has libdl], [
 CHECK_LIB([DL], [dl], [dlopen])])

CHECK_WANT([EXR], [auto], [use exr], [])

CHECK_WANT([OPENCV], [no], [opencv=sys/sta/dyn,git/tar=url], [
])

CHECK_WANT([NUMA], [auto], [system has libnuma], [
 CHECK_LIB([NUMA], [numa], [numa_alloc])])

CHECK_WANT([OPENEXR], [auto], [use openexr], [
 AC_MSG_CHECKING([for openexr available])
 saved_LIBS="$LIBS"
 saved_CXXFLAGS="$CXXFLAGS"
 # ilmbase libs
 LIBS=" -lIlmImf -lIlmThread -lIex -lpthread"
 CXXFLAGS="-I/usr/include/OpenEXR -I/usr/local/include/OpenEXR"
 AC_LANG_PUSH(C++)
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include "ImfChannelList.h"
#include "ImfChromaticities.h"
#include "ImfCompression.h"
#include "ImfIO.h"
#include "ImfInputFile.h"
#include "ImfOutputFile.h"
#include "ImfPixelType.h"
#include "ImfRgbaFile.h"
#include "ImfRgbaYca.h"
#include "ImfVersion.h"
class EXRIStream : public Imf::IStream { public:
EXRIStream() : Imf::IStream("mypath") {} };
class EXROStream : public Imf::OStream { public: 
EXROStream() : Imf::OStream("mypath") {} };
]])], [HAVE_OPENEXR=yes], [HAVE_OPENEXR=no])
 RESULT="$HAVE_OPENEXR"
 if test "x$RESULT" = "xyes"; then
   SHARED_openexr="$LIBS"
   CFG_CFLAGS+=" -I/usr/include/OpenEXR -I/usr/local/include/OpenEXR"
 fi
 LIBS="$saved_LIBS"
 CXXFLAGS="$saved_CXXFLAGS"
 AC_LANG_POP(C++)
 AC_MSG_RESULT([$HAVE_OPENEXR])
])
AC_SUBST([HAVE_OPENEXR])

# build global_config
OBJDIR=`uname -m`
AC_SUBST(OBJDIR)

echo "AC_HELP_STRING([Reason],[Package])"
echo "AC_HELP_STRING([------],[-------])"

AC_DEFUN([PKG_DISABLED],[
 PKG_$1="no"
 BUILD_$1=0
 AC_SUBST(BUILD_$1)
 echo "AC_HELP_STRING([disabled],[$1])"
])
AC_DEFUN([PKG_SHARED],[
 BUILD_$1=0
 AC_SUBST(BUILD_$1)
 SHARED_LIBS+="$SHARED_$1"
])

AC_DEFUN([PKG_STATIC],[
 if test "x$WANT_CIN_3RDPARTY" != "xno"; then
  PKG_$1="yes"
  BUILD_$1=1
  AC_SUBST(BUILD_$1)
  STATIC_BLDS+=" [$1]"
  echo "AC_HELP_STRING([$2],[$1])"
 fi
])

AC_DEFUN([PKG_FORCED],[PKG_STATIC([$1],[forced])])

if test "x$WANT_CIN_3RDPARTY" != "xno"; then
# order matters
for dep in \
        ffmpeg/twolame ffmpeg/lame ffmpeg/openjpeg \
        ffmpeg/libvorbis ffmpeg/libtheora ffmpeg/x264 ffmpeg/x265 \
        libiec61883/libraw1394 libavc1394/librom1394 \
        openexr/ilmbase ilmbase/libogg \
        libtheora/libogg libtheora/libvorbis ; do
  lib=`dirname $dep`;  needs=`basename $dep`
  eval pkg_lib="\$PKG_$lib"; eval needs_lib="\$HAVE_$needs";
  if test "x$pkg_lib" != "xno"; then
    eval "PKG_$needs=yes"
  fi
done
fi

AC_DEFUN([PKG_PROVIDE],[
if test "x$BUILD_$1" = "x" ; then
 if test "x$PKG_$1" = "xyes"; then
  PKG_FORCED([$1])
 elif test "x$2" = "xno" -o "x$PKG_$1" = "xno"; then
  PKG_DISABLED([$1])
 elif test "x$WANT_STATIC_BUILD" = "xyes"; then
  PKG_STATIC([$1],[static])
 elif test "x$HAVE_$1" = "xyes"; then
  PKG_SHARED([$1])
 else
  PKG_STATIC([$1],[needed])
 fi
fi
])

PKG_PROVIDE([a52dec])
PKG_PROVIDE([djbfft])
PKG_PROVIDE([encore])
PKG_PROVIDE([audiofile],[$WANT_ESOUND])
PKG_PROVIDE([esound],[$WANT_ESOUND])
PKG_PROVIDE([ffmpeg])
PKG_PROVIDE([fftw])
PKG_PROVIDE([flac])
PKG_PROVIDE([giflib])
PKG_PROVIDE([ilmbase], [$WANT_OPENEXR])
PKG_PROVIDE([ladspa],[$WANT_LADSPA_BUILD])
PKG_PROVIDE([lame])
PKG_PROVIDE([libavc1394],[$WANT_FIREWIRE])
PKG_PROVIDE([libraw1394],[$WANT_FIREWIRE])
PKG_PROVIDE([libiec61883],[$WANT_FIREWIRE])
PKG_PROVIDE([libdv],[$WANT_DV])
PKG_PROVIDE([libjpeg])
PKG_PROVIDE([libogg])
PKG_PROVIDE([libsndfile])
PKG_PROVIDE([libtheora])
PKG_PROVIDE([libuuid])
PKG_PROVIDE([libvorbis])
PKG_PROVIDE([mjpegtools])
PKG_PROVIDE([libaom])
PKG_PROVIDE([libwebp])
PKG_PROVIDE([openexr], [$WANT_OPENEXR])
PKG_PROVIDE([openjpeg])
PKG_PROVIDE([tiff])
PKG_PROVIDE([twolame])
PKG_PROVIDE([x264])
PKG_PROVIDE([x265])
PKG_PROVIDE([libvpx])
PKG_PROVIDE([opus])
PKG_PROVIDE([lv2], [$WANT_LV2])
PKG_PROVIDE([lilv], [$WANT_LV2])
PKG_PROVIDE([sratom], [$WANT_LV2])
PKG_PROVIDE([serd], [$WANT_LV2])
PKG_PROVIDE([sord], [$WANT_LV2])
PKG_PROVIDE([suil], [$WANT_LV2])

if test "x$WANT_LV2" = "xyes"; then
  if test "x$HAVE_lv2" = "xyes" -a "x$BUILD_lilv" = "x0"; then
    CFG_CFLAGS+=' -I/usr/include/lilv-0 -I/usr/local/include/lilv-0'
    CFG_CFLAGS+=' -I/usr/include/suil-0 -I/usr/local/include/suil-0'
  fi
fi

AC_SUBST(STATIC_BLDS)
for f in $SHARED_LIBS; do
  echo "AC_HELP_STRING([shared],[$f])"
done
AC_SUBST(SHARED_LIBS)
for f in $SYSTEM_LIBS; do
  echo "AC_HELP_STRING([system],[$f])"
done
AC_SUBST(SYSTEM_LIBS)

echo ""
if test "x$WANT_CIN_3RDPARTY" != "xno"; then
  CFG_CFLAGS+=" -DHAVE_CIN_3RDPARTY"
fi
for v in GL XFT XXF86VM OSS ALSA FIREWIRE DV DVB \
	 VIDEO4LINUX2 ESOUND PACTL OPENEXR LV2 \
	 COMMERCIAL LIBZMPEG SHUTTLE SHUTTLE_USB; do
  eval vv="\$WANT_$v"
  if test "x$vv" != "xno"; then
    CFG_CFLAGS+=" -DHAVE_$v"
    vv=with
  else
    vv=without
  fi
  v=`echo "$v" | tr "A-Z" "a-z"`
  echo "  using: $vv-$v"
done

echo "  using: with-jobs = $WANT_JOBS"
echo "  using: exec-name = $WANT_CIN"
echo "  using: with-cinlib = $WANT_CINLIB_DIR"
echo "  using: with-cindat = $WANT_CINDAT_DIR"
echo "  using: with-config-dir = $WANT_CONFIG_DIR"
echo "  using: with-browser = $WANT_CIN_BROWSER"
echo "  using: with-plugin-dir = $WANT_PLUGIN_DIR"
echo "  using: with-ladspa-dir = $WANT_LADSPA_DIR"
echo "  using: with-opencv = $WANT_OPENCV"
echo "  using: with-git-ffmpeg = $WANT_GIT_FFMPEG"
echo "  using: with-noelision = $WANT_NOELISION"
echo "  using: with-booby = $WANT_BOOBY"
echo "  using: with-libzmpeg = $WANT_LIBZMPEG"
echo "  using: with-commerical = $WANT_COMMERCIAL"
echo "  using: with-vaapi = $WANT_VAAPI"
echo "  using: with-vdpau = $WANT_VDPAU"
echo ""
echo "  using: thirdparty build = $WANT_CIN_3RDPARTY"
echo "  using: single-user  = $WANT_CINBIN_BUILD"
echo "  using: static-build = $WANT_STATIC_BUILD"
echo "  using: ladspa-build = $WANT_LADSPA_BUILD"
echo ""

# build extras
if test "x$HAVE_tiff" = "xyes"; then
  if test "x$HAVE_jbig" = "xyes"; then
    EXTRA_LIBS+=' -ljbig'
  fi
fi
if test "x$HAVE_NUMA" = "xyes"; then
  EXTRA_LIBS+=' -lnuma'
fi

FFMPEG_EXTRA_LDFLAGS=""
if test "x$WANT_VAAPI" != "xno" -a "x$HAVE_VAAPI" = "xyes"; then
  FFMPEG_EXTRA_LDFLAGS+=' -lva'
  EXTRA_LIBS+=' -lva'
  WANT_VAPPI="yes"
  CFG_WANTS+=" VAAPI"
  if test "x$HAVE_vaapi_x11" = "xyes"; then
    FFMPEG_EXTRA_LDFLAGS+=' -lva-x11'
    EXTRA_LIBS+=' -lva-x11'
  fi
  if test "x$HAVE_vaapi_drm" = "xyes"; then
    FFMPEG_EXTRA_LDFLAGS+=' -lva-drm'
    EXTRA_LIBS+=' -lva-drm'
  fi
fi
if test "x$WANT_VDPAU" != "xno" -a "x$HAVE_VDPAU" = "xyes"; then
  WANT_VDPAU="yes"
  CFG_WANTS+=" VDPAU"
fi

if test "x$HAVE_DL" = "xyes"; then
  EXTRA_LIBS+=' -ldl'
  FFMPEG_EXTRA_LDFLAGS+=' -ldl'
fi
if test "x$FFMPEG_EXTRA_LDFLAGS" != "x"; then
  FFMPEG_EXTRA_CFG+=' --extra-ldflags="'$FFMPEG_EXTRA_LDFLAGS'"'
fi

if test "x$WANT_BOOBY" != "xno"; then
  CFG_CFLAGS+=" -DBOOBY"
fi

# intel lock elision bugs
if test "x$WANT_NOELISION" != "xno"; then
  CFG_RPATH=`ls -1fd 2> /dev/null \
 /usr/lib/noelision \
 /lib/noelision \
 /usr/lib64/noelision \
 /lib64/noelision \
  | tail -1`
fi

AC_SUBST(WANT_CIN_3RDPARTY)
AC_SUBST(EXTRA_LIBS)
AC_SUBST(FFMPEG_EXTRA_CFG)
AC_SUBST(WANT_X264_HIDEPTH)
AC_SUBST(WANT_X265_HIDEPTH)

AC_SUBST(CFG_CFLAGS)
AC_SUBST(CFG_CXXFLAGS)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

exec > global_config

echo "SHELL := /bin/bash"
echo "export SHELL"
echo "WANT_JOBS := $WANT_JOBS"
echo ""
echo "OBJDIR := $OBJDIR"
echo "BINDIR := \$(TOPDIR)/bin"
echo "CINELERRA := \$(TOPDIR)/cinelerra"
echo "GUICAST := \$(TOPDIR)/guicast"
echo "LIBZMPEG3 := \$(TOPDIR)/libzmpeg3"
echo "MPEG2ENC := \$(TOPDIR)/mpeg2enc"
echo "MPLEXLO := \$(TOPDIR)/mplexlo"
echo "PLUGINS := \$(TOPDIR)/plugins"
echo "TDB := \$(TOPDIR)/db"
echo "THIRDPARTY := \$(TOPDIR)/thirdparty"
echo "EXTRA_LIBS := $EXTRA_LIBS"
echo "FFMPEG_EXTRA_CFG := $FFMPEG_EXTRA_CFG"
echo "export OBJDIR BINDIR CINELERRA GUICAST LIBZMPEG3"
echo "export MPEG2ENC MPLEXLO PLUGINS TDB"
echo "export THIRDPARTY EXTRA_LIBS FFMPEG_EXTRA_CFG"
echo ""

echo "WANT_CIN := $WANT_CIN"
CFG_WANTS+=" CIN_3RDPARTY LIBZMPEG COMMERCIAL"
for w in $CFG_WANTS; do ww=WANT_$w; echo "WANT_$w := ${!ww}"; done
echo ""

echo "ifndef CFLAGS_"
echo "ifdef CFLAGS"
echo "CFLAGS_ := \$(CFLAGS)"
echo "else"
echo "CFLAGS_ := ${CFLAGS:--g -O2}"
echo "endif"
echo "endif"
echo "ifndef CXXFLAGS_"
echo "ifdef CXXFLAGS"
echo "CXXFLAGS_ := \$(CXXFLAGS)"
echo "endif"
echo "endif"
echo "ifndef LDFLAGS_"
echo "ifdef LDFLAGS"
echo "LDFLAGS_ := \$(LDFLAGS)"
echo "endif"
echo "endif"

echo "CFLAGS := \$(CFLAGS_)"
echo "CFLAGS += '-DCIN=\"\$(WANT_CIN)\"'"
echo "CFLAGS += '-DCINLIB_DIR=\"$WANT_CINLIB_DIR\"'"
echo "CFLAGS += '-DCINDAT_DIR=\"$WANT_CINDAT_DIR\"'"
echo "CFLAGS += '-DCONFIG_DIR=\"$WANT_CONFIG_DIR\"'"
echo "CFLAGS += '-DPLUGIN_DIR=\"$WANT_PLUGIN_DIR\"'"
echo "CFLAGS += '-DLOCALE_DIR=\"$WANT_LOCALE_DIR\"'"
echo "CFLAGS += '-DLADSPA_DIR=\"$WANT_LADSPA_DIR\"'"
echo "CFLAGS += '-DCIN_BROWSER=\"$WANT_CIN_BROWSER\"'"
for flg in $CFG_CFLAGS; do echo "CFLAGS += $flg"; done
echo ""
if test "x$CFG_CXXFLAGS" != "x"; then
  for flg in $CFG_CXXFLAGS; do echo "CXXFLAGS += $flg"; done
  echo ""
fi
if test "x$CFG_LDFLAGS" != "x" ; then
  echo "LDFLAGS +=$CFG_LDFLAGS"
  echo ""
fi

if test "x$WANT_X264_HIDEPTH" = "xyes" ; then
  X264_CFG_PARAMS="$X264_CFG_PARAMS --bit-depth=10"
fi
if test "x$WANT_X265_HIDEPTH" = "xyes" ; then
  X265_CFG_PARAMS="$X265_CFG_PARAMS -DHIGH_BIT_DEPTH:BOOL=ON" # -DMAIN12:BOOL=ON"
fi

for pkg in $STATIC_PKGS; do
  eval pkg_lib="\$PKG_$pkg"
  if test "x$pkg_lib" = "xno"; then continue; fi;
  echo "static_pkgs += $pkg"
done
echo ""

for pkg in $STATIC_BLDS; do
  echo "static_blds += $pkg"
  ver_pkg=ver_$pkg
  echo "$ver_pkg := \$(THIRDPARTY)/${!ver_pkg}"
  inc_pkg=inc_$pkg
  echo "$inc_pkg := \\"
  for inc in ${!inc_pkg}; do
    echo "  -I\$($ver_pkg)/$inc \\"
  done
  echo ""
  echo "static_incs += \$(inc_$pkg)"
  lib_pkg=lib_$pkg
  echo "$lib_pkg := \\"
  for lib in ${!lib_pkg}; do
    echo "  \$($ver_pkg)/$lib \\"
  done
  echo ""
  echo "static_libs += \$($lib_pkg)"
  echo ""
done
echo ""

for lib in $SHARED_LIBS; do echo "shared_libs += $lib"; done
echo ""
for lib in $SYSTEM_LIBS; do echo "system_libs += $lib"; done
echo "export static_pkgs static_blds shared_libs system_libs"
echo ""

echo "thirdparty_libraries := \$(static_libs) \$(shared_libs)"

if test "x$CFG_RPATH" != "x"; then
  echo "libraries+= -Wl,-rpath=$CFG_RPATH"
fi

echo "libraries += -Wl,--start-group"
echo "libraries += \$(thirdparty_libraries)"
echo "libraries += \$(system_libs)"
echo "libraries += \$(EXTRA_LIBS)"
echo "libraries += -Wl,--end-group"
# -Wl,--start-group ... -Wl,--end-group does not work on ubuntu
echo "libraries += \$(thirdparty_libraries)"
echo "libraries += \$(system_libs)"
echo "libraries += \$(EXTRA_LIBS)"
echo ""
echo "export thirdparty_libraries libraries"
echo "export CFLAGS_ CXXFLAGS_ LDFLAGS_"
echo "unexport CFLAGS CXXFLAGS LDFLAGS"

if test "x$HAVE_tiff" = "xyes"; then
  if test "x$HAVE_jbig" != "xyes"; then
    echo "tiff.cfg_params+= --disable-jbig"
  fi
fi

if test "x$WANT_GIT_FFMPEG" != "xno"; then
  echo "ffmpeg.git := $WANT_GIT_FFMPEG"
fi
if test "x$X264_CFG_PARAMS" != "x" ; then
  echo "x264.cfg_params :=$X264_CFG_PARAMS --enable-static --enable-pic"
fi
if test "x$X265_CFG_PARAMS" != "x" ; then
  echo "x265.cfg_params :=$X265_CFG_PARAMS -DENABLE_SHARED=no"
fi
