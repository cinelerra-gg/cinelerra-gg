#!/bin/bash -x

if [ $# -ne 2 ]; then
  echo "usage: $0 <os> <typ>"
  echo "  os = centos | ubuntu | suse | debian"
  echo " typ = sta | dyn"
  exit 1
fi

dir="$1"
typ="$2"
path="/home"
bld="git-repo"
proj="cinelerra5"
base="cinelerra-5.1"

arch01="arch-20160601"
arch="arch-20161201"
cent01="centos-7.0-1406"
centos="centos-7.x-1611"
deb01="debian-8.4.0"
deb03="debian-8.7.1"
debian="debian-8.6.0"
fc23="fedora-23"
fc24="fedora-24"
fedora="fedora-25"
leap01="leap-42.1"
leap="leap-42.2"
mint01="mint-14.04.1"
mint="mint-18.1"
slk32="slk32-14.2"
slk64="slk64-14.2"
suse="opensuse-13.2"
ub14="ub14.04.1"
ub15="ub15.10"
ub16="ub16.04"
ub17="ub17.04"
ub1601="ub16.10"
ubuntu="ubuntu-14.04.1"

eval os="\${$dir}"
if [ -z "$os" ]; then
  echo "unknown os: $dir"
fi

if [ ! -d "$path/$dir/$bld/$proj.$typ/$base" ]; then
  echo "missing $bld/$proj.$typ/$base in $path/$dir"
  exit 1
fi

sfx=`uname -m`-`date +"%Y%m%d"`
if [ "$typ" = "sta" ]; then
  sfx="$sfx-static"
elif [ "$typ" != "dyn" ]; then
  echo "err: suffix must be [sta | dyn]"
  exit 1
fi

cd "$path/$dir/$bld/$proj.$typ/$base"
tar -C bin -cJf "../$base-$os-$sfx.txz" .
rm -f "$path/$dir/$base-$os-$sfx.txz"
mv "../$base-$os-$sfx.txz" "$path/$dir/."

