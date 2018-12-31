#!/bin/bash -x
# cd cincv;  cfg_cv.sh /path/cin5
# used git clone "git://git.cinelerra-gg.org/goodguy/cinelerra.git" "cinelerra5"
# ver: git checkout 135eb5f052a2f75e4df6c86511a94ec9586e096d (dec 30, 2016)
cin="$1"
THIRDPARTY=`pwd`/thirdparty
unset LIBS LDFLAGS CFLAGS CPPFLAGS CXXFLAGS
export ac_cv_header_xmmintrin_h=no
for f in ./quicktime/Makefile.am ./libmpeg3/Makefile.am; do
  sed -e 's/-Wl,--no-undefined//' -i $f
done
rm -rf thirdparty; cp -a $cin/thirdparty .
for f in configure.ac Makefile.am autogen.sh; do mv $f $f.cv; cp -a $cin/$f .; done
mv m4 m4.cv
rm -rf ./libzmpeg3 ./db
mkdir -p libzmpeg3 db db/utils mpeg2enc mplexlo

./autogen.sh
./configure --disable-static-build --without-ladspa-build \
  --enable-faac=yes --enable-faad2=yes --enable-a52dec=yes \
  --enable-mjpegtools=yes --enable-lame=yes --enable-x264=yes \
  --enable-libogg=auto --enable-libtheora=auto --enable-libvorbis=auto \
  --enable-openexr=auto --enable-libsndfile=auto --enable-libdv=auto \
  --enable-libjpeg=auto --enable-tiff=auto --enable-x264=auto \
  --enable-audiofile --disable-encore --disable-esound --disable-fdk \
  --disable-ffmpeg --disable-fftw --disable-flac --disable-giflib --disable-ilmbase \
  --disable-libavc1394 --disable-libraw1394 --disable-libiec61883 --disable-libvpx \
  --disable-openjpeg --disable-twolame --disable-x265

export CFG_VARS='CFLAGS+=" -fPIC"'; \
export MAK_VARS='CFLAGS+=" -fPIC"'; \
export CFG_PARAMS="--with-pic --enable-pic --disable-asm"; \

jobs=`make -s -C thirdparty val-WANT_JOBS`
make -C thirdparty -j$jobs

static_libs=`make -C thirdparty -s val-static_libs`
static_incs=`make -C thirdparty -s val-static_incs`

./autogen.sh clean
for f in configure.ac Makefile.am autogen.sh; do rm -f $f; mv $f.cv $f; done
mv m4.cv m4

LDFLAGS=`for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;/[^/]*$;;' | \
sort -u | while read d; do
 echo -n " -L$d";
done`
export LDFLAGS

LIBS=`for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;.*/;;' -e 's;lib\(.*\)\.a$;\1;' | \
sort -u | while read a; do
 echo -n " -l$a";
done`
LIBS+=" -lpthread"
export LIBS

export CFLAGS="$static_incs"
export CXXFLAGS="$static_incs"

if [ ! -f configure ]; then ./autogen.sh; fi
sed -e 's/^LIBX264_LIBS=""/#LIBX264_LIBS=""/' -i configure

export MJPEG_LIBS="-L$THIRDPARTY/mjpegtools-2.1.0/utils/.libs -lmjpegutils \
  -L$THIRDPARTY/mjpegtools-2.1.0/lavtools/.libs -llavfile \
  -L$THIRDPARTY/mjpegtools-2.1.0/lavtools/.libs -llavjpeg \
  -L$THIRDPARTY/mjpegtools-2.1.0/mpeg2enc/.libs -lmpeg2encpp \
  -L$THIRDPARTY/mjpegtools-2.1.0/mplex/.libs -lmplex2"
export MJPEG_CFLAGS="-I$THIRDPARTY/mjpegtools-2.1.0/. \
  -I$THIRDPARTY/mjpegtools-2.1.0/lavtools \
  -I$THIRDPARTY/mjpegtools-2.1.0/utils"

export LIBX264_CFLAGS="-I$THIRDPARTY/x264-snapshot-20160220-2245-stable/."
export LIBX264_LIBS="-L$THIRDPARTY/x264-snapshot-20160220-2245-stable/. -lx264"

for f in $MJPEG_LIBS $LIBX264_LIBS; do
  LIBS=`echo "$LIBS" | sed -e "s;[ ]*\<$f\>[ ]*; ;"`
done

echo LDFLAGS=$LDFLAGS
echo LIBS=$LIBS
echo CFLAGS=$CFLAGS

# -lmxxxxx dies, feed it a -lm early to prevent misformed parameters
export LIBS="-la52 -ldjbfft -lfaac -lfaad -lHalf -lIex -lIexMath -lIlmThread -lImath -llavfile -llavjpeg -lm -lmjpegutils -lmmxsse -lmp3lame -lmp4ff -lmpeg2encpp -lmpgdecoder -lmplex2 -logg -lvorbis -lvorbisenc -lvorbisfile -lx264 -lyuvfilters -lpthread -ldl"
# po Makefile construction error: skip it
touch po/Makefile.in.in

# uuid search path is not predictable, fake it
( cd thirdparty/libuuid-1.0.3/; ln -s . uuid )
# old linker scripts want .libs/.libs path, fake it:
find thirdparty/ -name .libs | while read f ; do ( cd $f; ln -s . .libs ); done

./configure

echo "Run:"
echo "export ac_cv_header_xmmintrin_h=no"
echo "export THIRDPARTY=\"$THIRDPARTY\""
exit

# have to rebuild these by hand
# have to get rid of  -Wl,--no-undefined"
# cd plugins/libeffecttv/.libs/; \rm -f libeffecttv.a; ar r libeffecttv.a effecttv.o
# cd quicktime/encore50/.libs/; \rm -f libencore.a; ar r libencore.a *.o
# cd quicktime; vi Makefile; remove -Wl,--no-undefined from:, make
#   libquicktimecv_la_LDFLAGS = -version-info 1:0:0 -release 1.6.0

# shares do not want thirdparty link data:
( cd libmpeg3
  make clean
  make LIBS= \
   LDFLAGS="-L$THIRDPARTY/a52dec-0.7.4/liba52/.libs \
   -L$THIRDPARTY/djbfft-0.76  -ldjbfft" )
( cd guicast
  make clean
  make LIBS= LDFLAGS= )
( cd plugins
  for f in */Makefile; do
    cp $f $f.sav1; sed -e "/^LIBS\>/d" -e "/^LDFLAGS\>/d" -i $f
  done
  make clean
  make  )

#make -j$jobs >& log
#make install DESTDIR=`pwd` >> log 2>&1
#export LD_LIBRARY_PATH=`pwd`/usr/local/lib
#cd cinelerra
#gdb ./.libs/cinelerra

