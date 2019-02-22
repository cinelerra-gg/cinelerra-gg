#!/bin/bash
#  need lv2 plugins (eg. lv2-calf-plugin)

if [ `id -u` -ne 0 ]; then
  echo "you must be root"
fi

if [ $# -ne 1 ]; then
  echo "usage: $0 <os>"
  echo "  <os> = [centos | suse | ubuntu]"
fi

dir="$1"


case "$dir" in
"centos")
  yum -y install nasm libavc1394-devel libusbx-devel flac-devel \
    libjpeg-devel libdv-devel libdvdnav-devel libdvdread-devel \
    libtheora-devel libiec61883-devel uuid-devel giflib-devel \
    ncurses-devel ilmbase-devel fftw-devel OpenEXR-devel \
    libsndfile-devel libXft-devel libXinerama-devel libXv-devel \
    xorg-x11-fonts-misc xorg-x11-fonts-cyrillic xorg-x11-fonts-Type1 \
    xorg-x11-fonts-ISO8859-1-100dpi xorg-x11-fonts-ISO8859-1-75dpi \
    libpng-devel bzip2-devel zlib-devel kernel-headers libtiff-devel \
    libavc1394 festival-devel libiec61883-devel flac-devel inkscape \
    libsndfile-devel libtheora-devel linux-firmware ivtv-firmware \
    libvorbis-devel texinfo xz-devel lzma-devel cmake udftools git \
    autoconf automake rpm-build jbigkit-devel libvdpau-devel alsa-lib-devel \
    gtk2-devel
    yasm=yasm-1.3.0-3.fc24.x86_64.rpm
    release=http://archives.fedoraproject.org/pub/fedora/linux/releases/24
    url=$release/Everything/x86_64/os/Packages/y/$yasm
    wget -P /tmp $url
    yum -y install /tmp/$yasm
    rm -f /tmp/$yasm
  ;;
"fedora")
  dnf install groups "Development Tools"
  dnf -y --best --allowerasing \
    install nasm yasm libavc1394-devel libusbx-devel flac-devel \
    libjpeg-devel libdv-devel libdvdnav-devel libdvdread-devel \
    libtheora-devel libiec61883-devel esound-devel uuid-devel \
    giflib-devel ncurses-devel ilmbase-devel fftw-devel OpenEXR-devel \
    libsndfile-devel libXft-devel libXinerama-devel libXv-devel \
    xorg-x11-fonts-misc xorg-x11-fonts-cyrillic xorg-x11-fonts-Type1 \
    xorg-x11-fonts-ISO8859-1-100dpi xorg-x11-fonts-ISO8859-1-75dpi \
    libpng-devel bzip2-devel zlib-devel kernel-headers libavc1394 \
    festival-devel libdc1394-devel libiec61883-devel esound-devel \
    flac-devel libsndfile-devel libtheora-devel linux-firmware \
    ivtv-firmware libvorbis-devel texinfo xz-devel lzma-devel cmake git \
    ctags patch gcc-c++ perl-XML-XPath libtiff-devel python dvdauthor \
    gettext-devel inkscape udftools autoconf automake numactl-devel \
    jbigkit-devel libvdpau-devel gtk2-devel
  ;;
"suse" | "leap")
  zypper -n install nasm gcc gcc-c++ zlib-devel texinfo libpng16-devel \
    freeglut-devel libXv-devel alsa-devel libbz2-devel ncurses-devel \
    libXinerama-devel freetype-devel libXft-devel giflib-devel ctags \
    bitstream-vera-fonts xorg-x11-fonts-core xorg-x11-fonts dejavu-fonts \
    openexr-devel libavc1394-devel festival-devel libjpeg8-devel libdv-devel \
    libdvdnav-devel libdvdread-devel libiec61883-devel libuuid-devel \
    ilmbase-devel fftw3-devel libsndfile-devel libtheora-devel flac-devel \
    libtiff-devel inkscape cmake patch libnuma-devel lzma-devel udftools git \
    yasm autoconf automake rpm-build libjbig-devel libvdpau-devel gtk2-devel \
    libusb-1_0-devel
    if [ ! -f /usr/lib64/libtermcap.so ]; then
      ln -s libtermcap.so.2 /usr/lib64/libtermcap.so
    fi
  ;;
#debian 32bit: export ac_cv_header_xmmintrin_h=no
"debian")
  apt-get -f -y install apt-file sox nasm yasm g++ build-essential zlib1g-dev \
    texinfo libpng12-dev freeglut3-dev libxv-dev libasound2-dev libbz2-dev \
    libncurses5-dev libxinerama-dev libfreetype6-dev libxft-dev libgif-dev \
    libtiff5-dev exuberant-ctags ttf-bitstream-vera xfonts-75dpi xfonts-100dpi \
    fonts-dejavu libopenexr-dev festival libfftw3-dev gdb libusb-1.0-0-dev \
    libdc1394-22-dev libflac-dev libjbig-dev libvdpau-dev \
    inkscape libsndfile1-dev libtheora-dev cmake udftools libxml2-utils git \
    autoconf automake debhelper libgtk2.0-dev
  ;;
#"ub16-10")
#  apt-get -y install libx264-dev libx265-dev libvpx-dev libmjpegtools-dev
"ubuntu" | "mint" | "ub14" | "ub15" | "ub16" | "ub17" | "ub18" )
  apt-get -y install apt-file sox nasm yasm g++ build-essential libz-dev \
    texinfo libpng-dev freeglut3-dev libxv-dev libasound2-dev libbz2-dev \
    libncurses5-dev libxinerama-dev libfreetype6-dev libxft-dev libgif-dev \
    libtiff5-dev exuberant-ctags ttf-bitstream-vera xfonts-75dpi xfonts-100dpi \
    fonts-dejavu libopenexr-dev libavc1394-dev festival-dev fftw3-dev gdb \
    libdc1394-22-dev libiec61883-dev libflac-dev libjbig-dev libusb-1.0-0-dev \
    libvdpau-dev libsndfile1-dev libtheora-dev cmake udftools libxml2-utils \
    git inkscape autoconf automake debhelper libgtk2.0-dev
  ;;
 *)
  echo "unknown os: $dir"
  exit 1;
  ;;
esac

mkdir -p "/home/$dir/git-repo"
chmod a+rwx -R "/home/$dir/git-repo"

