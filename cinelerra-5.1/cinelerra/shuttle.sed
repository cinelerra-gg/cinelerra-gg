:n1 # just if these
s/^#ifdef XK_MISCELLANY\>.*//p; t n2
s/^#ifdef XK_XKB_KEYS\>.*//p; t n2
s/^#ifdef XK_LATIN1\>.*//p; t n2
n; b n1
:n2 # until endif
s/^#endif\>.*//p; t n1
s/^#define \([^[:space:]]*\).*$/{ "\1", \1 }, /p
n; b n2
