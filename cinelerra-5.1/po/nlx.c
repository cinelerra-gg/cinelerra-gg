#include <stdio.h>
#include <string.h>
#include <libintl.h>

#define _(s) gettext(s)
#define gettext_noop(s) s
#define N_(s) gettext_noop(s)

int lastch = 0;
int linenum = 0;
int start_line = 1;
int in_quote = 0;

#define BFRSZ 4096
char bfr[BFRSZ], *bp = 0;
char sym[BFRSZ];
char sep[BFRSZ];
char str[BFRSZ];

FILE *ifp = 0, *ofp = 0;

int ngetc()
{
  if( lastch < 0 ) return -1;
  if ((lastch=getc(ifp)) == '\n') linenum++;
  return lastch;
}

void nungetc(int c)
{
  if( c < 0 ) return;
  if( c == '\n' ) --linenum;
  ungetc(c, ifp);
}

// return uncommented char
int cch()
{
  for(;;) {
    int c = ngetc();
    if( in_quote ||  c != '/' ) return c;
    c = ngetc();
    switch(c) {
    case '*':
      *bp++ = '/';  *bp++ = '*';
      c = ' ';
      for(;;) {
        int ch = c;  c = ngetc();
        if( c < 0 ) return c;
        *bp++ = c;
        if( ch == '*' && c == '/') break;
      }
      continue;
    case '/':
      *bp++ = '/';  *bp++ = '/';
      for(;;) {
        c = ngetc();
        if( c < 0 ) return c;
        *bp++ = c;
        if( c == '\n') break;
      }
      continue;
    default:
      nungetc(c);
      return '/';
    }
  }
}

// skip preprocessor lines
int nch()
{
  int c = 0;
  for(;;) {
    int ch = c;  c = cch();
    if( c < 0 ) return c;
    if( !start_line || c != '#' ) break;
    do {
      *bp++ = c;
      ch = c;  c = ngetc();
      if( c < 0 ) return c;
    } while( c != '\n' || ch == '\\' );
    *bp++ = c;
  }
  start_line = c == '\n' ? 1 : 0;
  return c;
}

int is_idch(int c)
{
  if( c < '0' ) return 0;
  if( c <= '9' ) return 1;
  if( c < 'A' ) return 0;
  if( c <= 'Z' ) return 1;
  if( c == '_' ) return 1;
  if( c < 'a' ) return 0;
  if( c <= 'z' ) return 1;
  return 0;
}

int is_ws(int c)
{
  if( c <= ' ' ) return 1;
  return 0;
}

// scan next seperator + symbol
int nsym()
{
  int c = nch();
  if( c < 0 ) return c;
  if( c == '\"' || c == '\'' ) return c;
  nungetc(c);
  char *sp = sym;
  for(;;) {
    c = nch();
    if( c < 0 ) return c;
    if( c == '"' || c == '\'' ) break;
    if( !is_idch(c) ) break;
    *bp++ = c;
    *sp++ = c;
  }
  *sp = 0;
  nungetc(c);
  sp = sep;
  for(;;) {
    c = nch();
    if( c < 0 ) return c;
    if( c == '\"' || c == '\'' ) break;
    if( is_idch(c) ) break;
    *bp++ = c;
    if( !is_ws(c) )
      *sp++ = c;
  }
  *sp = 0;
  nungetc(c);
  return 0;
}

// sym has preceeding symbol string
// sep has non-ws seperator string
// bfr has "string" in it
int need_nl()
{
  if( strlen(bfr) < 2 ) return 0;
  if( !strcmp(sym,"_") && !strcmp(sep,"(") ) return 0;
// add protected names here
  if( !strcmp(sym,"get") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"get_property") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"set_property") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"set_title") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"title_is") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"GET_DEFAULT") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"UPDATE_DEFAULT") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"TRACE") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"lock") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"unlock") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"lock_window") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"unlock_window") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"update") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"new_image") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"get_image") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"get_image_set") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"Mutex") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"SceneNode") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"Garbage") && !strcmp(sep,"(") ) return 0;
  if( !strcmp(sym,"dbmsg") && !strcmp(sep,"(") ) return 0;
  if( strstr(bfr, "::") || !strncmp(bfr, "/dev/", 5) ) return 0;
  if( !strcmp(bfr,"toc") || !strcmp(bfr,".toc") ) return 0;
  if( !strcmp(bfr,"idx") || !strcmp(bfr,".idx") ) return 0;
  if( !strcmp(bfr,"ads") || !strcmp(bfr,".ads") ) return 0;
  char *bp;
  for( bp=bfr; *bp; ++bp ) {
    if( is_ws(*bp) ) continue;
    if( *bp == '\\' ) { ++bp;  continue; }
    if( *bp != '%' ) break;
    while( bp[1] && !strchr("diojxXeEfFgFaAcsCSpnm%",*++bp) );
  }
  if( !*bp ) return 0;
  return 1;
}

int main(int ac, char **av)
{
  int c;
  ifp = stdin;
  ofp = stdout;
  for(;;) {
    bp = &bfr[0];  *bp = 0;
    c = nsym();
    *bp = 0;
    if( c < 0 ) break;
    fputs(bfr, ofp);
    bp = &bfr[0];  *bp = 0;
    if( c == '\"' ) {
      in_quote = c;
      while( (c=nch()) >= 0 ) {
        if( c == in_quote ) break;
        if( c == '\\' ) {
          *bp++ = c;  c = nch();
          if( c < 0 ) break;
        }
        *bp++ = c;
      }
      in_quote = 0;
      *bp = 0;
      if( c < 0 ) break;
      int do_nl = need_nl();
      if( do_nl ) {
        fputc('_', ofp);
        fputc('(', ofp);
      }
      fputc('"', ofp);
      fputs(bfr, ofp);
      fputc('"', ofp);
      if( do_nl ) {
        fputc(')', ofp);
      }
    }
    else if( c == '\'' ) {
      in_quote = c;
      while( (c=nch()) >= 0 ) {
        if( c == in_quote ) break;
        if( c == '\\' ) {
          *bp++ = c;  c = nch();
          if( c < 0 ) break;
        }
        *bp++ = c;
      }
      in_quote = 0;
      *bp = 0;
      fputc('\'', ofp);
      fputs(bfr, ofp);
      fputc('\'', ofp);
      if( c < 0 ) break;
    }
    else
      continue;
  }
  fputs(bfr, ofp);
  return 0;
}

