#!/bin/bash
# This is a little utility that make all pngs that this theme needs

if [ -z "$(which inkscape)" ]; then
  echo "Error: program inkscape not found."
  exit 1
fi

var="$@"
if [ -z "$var" ]; then
  SOURCE=$(ls -1 Source | grep '\.svg$' | grep -v '_all.svg$\|^0\.[0-9A-Za-z].*\.svg$')
  echo $(echo "$SOURCE" | wc -l) SVG files found.
  Z=0
  for i in $SOURCE ; do
      let Z+=1
      echo "  [$Z] Convert: $i ..."
      inkscape -e $(basename "$i" .svg).png Source/"$i"
  done
else
    for i in $@; do
        case "$i" in
            *.svg) inkscape -e $(basename "$i" .svg).png "$i" ;;
            *) echo "Error: This program only converts SVG to PNG." ;;
        esac
    done
fi
