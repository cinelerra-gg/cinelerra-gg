#!/bin/bash -x

dir="$1"
shift
path="/home"
bld="git-repo"
proj="cinelerra5"
base="cinelerra-5.1"

if [ ! -d "$path/$dir/$bld" ]; then
  echo "$bld missing in $path/$dir"
  exit 1
fi

cd "$path/$dir/$bld"
rm -rf "$proj.dyn"
mkdir "$proj.dyn"

git clone "git://git.cinelerra-gg.org/goodguy/cinelerra.git" "$proj.dyn"
#rsh host tar -C "/mnt0/$proj" -cf - "$base" | tar -C "$proj.dyn" -xf -
if [ $? -ne 0 ]; then
  echo "git clone $proj failed"
  exit 1
fi

cd "$proj.dyn/$base"
{
./autogen.sh && \
./configure --with-single-user --disable-static-build && \
make $@ && \
make install
} 2>&1 | tee log || true

echo "finished: scanning log for ***"
grep -ai "\*\*\*.*error" log | head

