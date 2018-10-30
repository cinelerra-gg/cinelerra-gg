#!/bin/bash
# this script is used on c++ source to modify the _()
#  gettext calls to use msgqual(ctxt,id) instead.
# local context regions are defined by using:
#
# #undef MSGQUAL
# #define MSGQUAL "qual_id"
# ... code with _() ...
# #undef MSGQUAL
# #define MSGQUAL 0
#

cin_dir=`mktemp -d -p /tmp cin_XXXXXX`
trap "rm -rf '$cin_dir'" EXIT
mkdir -p "$cin_dir"

echo 1>&2 "edit"
for d in guicast cinelerra plugins/*; do
  if [ ! -d "$d" ]; then continue; fi
  mkdir -p "$cin_dir/$d"
  ls -1 "$d"/*.[Ch] "$d"/*.inc 2> /dev/null
done | while read f ; do
#qualifier is reset using #define MSGQUAL "qual_id"
#this changes:
#   code C_("xxx") [... code _("yyy")]
#to:
#   code D_("qual_id#xxx") [... code D_("qual_id#yyy")]
  bn=${f##*/}; fn=${bn%.*}
  sed -n "$f" > "$cin_dir/$f" -f - <<<'1,1{x; s/.*/D_("'$fn'#/; x}; t n1
:n1 s/^\(#define MSGQUAL[ 	]\)/\1/; t n4
:n2 s/\<C_("/\
/; t n3; P; d; n; b n1
:n3 G; s/^\(.*\)\
\(.*\)\
\(.*\)$/\1\3\2/; b n2
:n4 p; s/^#define MSGQUAL[       ]*"\(.*\)"$/D_("\1#/; t n5
s/^.*$/_("/; h; d; n; b n1
:n5 h; d; n; b n1
'
done

#scan src and generate cin.po
echo 1>&2 "scan"
cd "$cin_dir"
for d in guicast cinelerra plugins/*; do
  if [ ! -d "$d" ]; then continue; fi
  ls -1 $d/*.[Ch] $d/*.inc 2> /dev/null
done | xgettext --no-wrap -L C++ -k_ -kN_ -kD_ -f - -o -

echo 1>&2 "done"

