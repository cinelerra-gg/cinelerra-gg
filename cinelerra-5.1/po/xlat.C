#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <map>

#define MX_STR 8192

// test csv    ./a.out csv < data.csv
// test po     ./a.out  po < data.po
// get strings ./a.out key  < xgettext.po
// gen xlation ./a.out xlat < xgettext.po xlat.csv
// gen xlation ./a.out xlat < xgettext.po text,xlat ...
// gen xupdate ./a.out xlat < xgettext.po xlat.csv newer.csv ... newest.csv

unsigned int wnext(uint8_t *&bp)
{
  unsigned int ch = *bp++;
  if( ch >= 0x80 ) {
    static const unsigned char byts[] = {
      1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5,
    };
    int i = ch - 0xc0;
    int n = i<0 ? 0 : byts[i/4];
    for( i=n; --i>=0 && *bp>=0x80; ch+=*bp++ ) ch <<= 6;
    static const unsigned int ofs[6] = {
      0x00000000U, 0x00003080U, 0x000E2080U,
      0x03C82080U, 0xFA082080U, 0x82082080U
    };
    ch = i<0 ?  ch-ofs[n] : '?';
  }
  return ch;
}

int wnext(uint8_t *&bp, unsigned int ch)
{
  if( ch < 0x00000080 ) { *bp++ = ch;  return 1; }
  int n = ch < 0x00000800 ? 2 : ch < 0x00010000 ? 3 :
          ch < 0x00200000 ? 4 : ch < 0x04000000 ? 5 : 6;
  int m = (0xff00 >> n), i = n-1;
  *bp++ = (ch>>(6*i)) | m;
  while( --i >= 0 ) *bp++ = ((ch>>(6*i)) & 0x3f) | 0x80;
  return n;
}

using namespace std;

//csv = comma seperated value file
#define SEP ','
static bool is_sep(int ch) { return ch == SEP; }

static bool is_opnr(int ch)
{
  if( ch ==  '\"'  ) return true;
  if( ch == 0xab ) return true;
  if( ch == 0xbb ) return true;
  if( ch == 0x300c ) return true;
  if( ch == 0x300d ) return true;
  return 0;
}

// converts libreoffice csv stuttered quoted string (with quotes attached)
//  quote marks only
static void xlat1(uint8_t *&in, uint8_t *out)
{
  uint8_t *ibp = in, *obp = out;
  unsigned ch, lch = 0;
  if( (ch=wnext(in)) == '\"' ) { 
    bool is_nested = in[0] == '\"' && in[1] == '\"';
    while( (ch=wnext(in)) != 0 ) {
      if( ch == '\"' && lch != '\\' ) {
        uint8_t *bp = in;
        unsigned nch = wnext(in);
        if( nch != '\"' ) { in = bp;  break; }
      }
      wnext(out, lch = ch);
    }
    if( is_nested && ch == '"' ) {
      while( out > obp && *(out-1) == ' ' ) --out;
    }
  }
  if( ch != '"' ) {
    in = ibp;  out = obp;
    while( (ch=wnext(in)) && !is_sep(ch) ) wnext(out,ch);
  }
  *out = 0;
}

static inline unsigned gch(uint8_t *&in) {
  unsigned ch = wnext(in);
  if( ch == '\\' ) {
    switch( (ch=*in++) ) {
    case 'a':  ch = '\a';  break;
    case 'b':  ch = '\b';  break;
    case 'f':  ch = '\f';  break;
    case 'n':  ch = '\n';  break;
    case 'r':  ch = '\r';  break;
    case 't':  ch = '\t';  break;
    case 'v':  ch = '\v';  break;
    }
  }
  return ch;
}

// converts string (with opn/cls attached) to c string
static void xlat2(uint8_t *in, uint8_t *out)
{
  unsigned lch = gch(in), sep = 0, rch = 0, ch;
  if( lch ) {
    if( is_opnr(lch) ) {
      for( uint8_t *ip=in; (ch=gch(ip))!=0; rch=ch );
      if( lch == rch ) { sep = lch;  lch = gch(in); }
    }
    while( (ch=gch(in)) != 0 ) {
      wnext(out, lch);  lch = ch;
    }
    if( !sep ) wnext(out, lch);
  }
  *out = 0;
}

int brkput = 0;

// converts c++ string to c string text
static void xlat3(const char *cp, uint8_t *out)
{
  wnext(out, '\"');
  unsigned ch;
  uint8_t *bp = (uint8_t*)cp;
  while( (ch=wnext(bp)) != 0 ) {
    switch( ch ) {
    case '"':   ch = '\"'; break;
    case '\a':  ch = 'a';  break;
    case '\b':  ch = 'b';  break;
    case '\f':  ch = 'f';  break;
    case '\n':  ch = 'n';  break;
    case '\r':  ch = 'r';  break;
    case '\t':  ch = 't';  break;
    case '\v':  ch = 'v';  break;
    default: wnext(out,ch);  continue;
    }
    wnext(out,'\\');
    wnext(out, ch);
    if( brkput && ch == 'n' && *bp ) {
      wnext(out, '\"');
      wnext(out, '\n');
      wnext(out, '\"');
    }
  }
  wnext(out, '\"');
  wnext(out, 0);
}

// converts c++ string to csv string text
static void xlat4(const char *cp, uint8_t *out)
{
  wnext(out, '\"');
  unsigned ch;
  uint8_t *bp = (uint8_t*)cp;
  while( (ch=wnext(bp)) != 0 ) {
    switch( ch ) {
    case '\a':  ch = 'a';  break;
    case '\b':  ch = 'b';  break;
    case '\f':  ch = 'f';  break;
    case '\n':  ch = 'n';  break;
    case '\r':  ch = 'r';  break;
    case '\t':  ch = 't';  break;
    case '\v':  ch = 'v';  break;
    case '\"': break;
    default: wnext(out,ch);  continue;
    }
    wnext(out,'\\');
    wnext(out,ch);
  }
  wnext(out, '\"');
  wnext(out, 0);
}

// parses input to c++ string
static string xlat(uint8_t *&in)
{
  uint8_t bfr[MX_STR];  bfr[0] = 0;  xlat1(in, bfr);
  uint8_t str[MX_STR];  str[0] = 0;  xlat2(bfr, str);
  return string((const char*)str);
}

class tstring : public string {
public:
  bool ok;
  tstring(const char *sp, bool k) { string::assign(sp); ok = k; }
};

typedef map<string,tstring> Trans;
static Trans trans;

static inline bool prefix_is(uint8_t *bp, const char *cp)
{
  return !strncmp((const char *)bp, cp, strlen(cp));
}
static inline uint8_t *bgets(uint8_t *bp, int len, FILE *fp)
{
  uint8_t *ret = (uint8_t*)fgets((char*)bp, len, fp);
  if( ret ) {
    int len = strlen((char *)bp);
    if( len > 0 && bp[len-1] == '\n' ) bp[len-1] = 0;
  }
  return ret;
}
static inline int bputs(uint8_t *bp, FILE *fp)
{
  if( !fp ) return 0;
  fputs((const char*)bp, fp);
  fputc('\n',fp);
  int n = 1;
  while( *bp ) if( *bp++ == '\n' ) ++n;
  return n;
}
static inline int bput(uint8_t *bp, FILE *fp)
{
  if( !fp ) return 0;
  fputs((const char*)bp, fp);
  return 1;
}

static bool goog = false;
static bool nocmts = false;

static inline bool is_nlin(unsigned ch, uint8_t *bp)
{
  return ch == ' ' && bp[0] == '\\' && bp[1] == ' ' && ( bp[2] == 'n' || bp[2] == 'N' );
}

static inline bool is_ccln(unsigned ch, uint8_t *bp)
{
  return ch == ' ' && bp[0] == ':' && bp[1] == ':' && bp[2] == ' ';
}

static inline bool is_quot(unsigned ch, uint8_t *bp)
{
  return ch == ' ' && bp[0] == '\\' && bp[1] == ' ' && bp[2] == '"';
}

static inline bool is_colon(unsigned ch)
{
  return ch == 0xff1a;
}

static inline bool is_per(unsigned ch)
{
  if( ch == '%' ) return true;
  if( ch == 0xff05 ) return true;
  return false;
}

static unsigned fmt_flds = 0;

enum { fmt_flg=1, fmt_wid=2, fmt_prc=4, fmt_len=8, fmt_cnv=16, };

static int is_flags(uint8_t *fp)
{
  if( (fmt_flds & fmt_flg) != 0 ) return 0;
  if( !strchr("#0-+ I", *fp) ) return 0;
  fmt_flds |= fmt_flg;
  return 1;
}

static int is_width(uint8_t *fp)
{
  if( (fmt_flds & fmt_wid) != 0 ) return 0;
  if( *fp != '*' && *fp < '0' && *fp > '9' ) return 0;
  fmt_flds |= fmt_wid;
  uint8_t *bp = fp;
  bool argno = *fp++ == '*';
  while( *fp >= '0' && *fp <= '9' ) ++fp;
  if( argno && *fp++ != '$' ) return 1;
  return fp - bp;
}

static int is_prec(uint8_t *fp)
{
  if( (fmt_flds & fmt_prc) != 0 ) return 0;
  if( *fp != '.' ) return 0;
  fmt_flds |= fmt_prc;
  if( *fp != '*' && *fp < '0' && *fp > '9' ) return 0;
  uint8_t *bp = fp;
  bool argno = *fp++ == '*';
  while( *fp >= '0' && *fp <= '9' ) ++fp;
  if( argno && *fp++ != '$' ) return 1;
  return fp - bp;
}

static int is_len(uint8_t *fp)
{
  if( (fmt_flds & fmt_len) != 0 ) return 0;
  if( !strchr("hlLqjzt", *fp) ) return 0;
  fmt_flds |= fmt_len;
  if( fp[0] == 'h' && fp[1] == 'h' ) return 2;
  if( fp[0] == 'l' && fp[1] == 'l' ) return 2;
  return 1;
}

static int is_conv(uint8_t *fp)
{
  if( !strchr("diouxXeEfFgGaAscCSpnm", *fp) ) return 0;
  return 1;
}


static inline int fmt_spec(uint8_t *fp)
{
  if( !*fp ) return 0;
  if( is_per(*fp) ) return 1;
  fmt_flds = 0;
  uint8_t *bp = fp;
  while( !is_conv(fp) ) {
    int len;
    if( !(len=is_flags(fp)) && !(len=is_width(fp)) &&
        !(len=is_prec(fp))  && !(len=is_len(fp)) ) return 0;
    fp += len;
  }
  return fp - bp + 1;
}

static bool chkfmt(int no, uint8_t *ap, uint8_t *bp, uint8_t *cp)
{
  bool ret = true;
  uint8_t *asp = ap, *bsp = bp;
  int n = 0;
  uint8_t *bep = bp;
  unsigned bpr = 0, bch = wnext(bp);
  for( ; bch!=0; bch=wnext(bp) ) {
    if( goog && is_opnr(bch) ) ++n;
    bep = bp;  bpr = bch;
  }

  // trim solitary opnrs on ends b
  if( goog && ( n != 1 || !is_opnr(bpr) ) ) bep = bp;
  bp = bsp;  bch = wnext(bp);
  if( goog && ( n == 1 && is_opnr(bch) ) ) bch = wnext(bp);

  unsigned apr = 0, ach = wnext(ap);
  apr = bpr = 0;
  while( ach != 0 && bch != 0 ) {
    // move to % on a
    while( ach != 0 && !is_per(ach) ) {
      apr = ach;  ach = wnext(ap);
    }
    // move to % on b
    while( bch != 0 && !is_per(bch) ) {
      if( goog ) { // google xlat recoginizers
        if( is_nlin(bch, bp) ) {
          bch = '\n';  bp += 3;
        }
        else if( is_ccln(bch, bp) ) {
          wnext(cp, bch=':');  bp += 3;
        }
        else if( is_quot(bch, bp) ) {
          bch = '\"';  bp += 3;
        }
        else if( is_colon(bch) ) {
          bch = ':';
        }
      }
      wnext(cp,bch);  bpr = bch;
      bch = bp >= bep ? 0 : wnext(bp);
    }
    if( !ach || !bch ) break;
    if( !*ap && !*bp ) break;
    // if % on a and % on b and is fmt_spec
    if( is_per(ach) && is_per(bch) && (n=fmt_spec(ap)) > 0 ) {
      if( apr && apr != bpr ) wnext(cp,apr);
      wnext(cp,ach);  apr = ach;  ach = wnext(ap);
      // copy format data from a
      while( ach != 0 && --n >= 0 ) {
        wnext(cp, ach);  apr = ach;  ach = wnext(ap);
      }
      bpr = bch;  bch = bp >= bep ? 0 : wnext(bp);
      if( apr == '%' && bch == '%' ) {
        // copy %% format data from b
        bpr = bch;
        bch = bp >= bep ? 0 : wnext(bp);
      }
      else {
        // skip format data from b (ignore case)
        while( bch != 0 && ((bpr ^ apr) & ~('a'-'A')) ) {
          bpr = bch;
          bch = bp >= bep ? 0 : wnext(bp);
        }
        // hit eol and didn't find end of spec on b
        if( !bch && ((bpr ^ apr) & ~('a'-'A')) != 0 ) {
          fprintf(stderr, "line %d: missed spec: %s\n", no, (char*)asp);
          ret = false;
        }
      }
    }
    else {
      fprintf(stderr, "line %d: missed fmt: %s\n", no, (char*)asp);
      wnext(cp, bch);
      apr = ach;  ach = wnext(ap);
      bpr = bch;  bch = bp >= bep ? 0 : wnext(bp);
      ret = false;
    }
  }
  while( bch != 0 ) {
    wnext(cp, bch);  bpr = bch;
    bch = bp >= bep ? 0 : wnext(bp);
  }
  if( apr == '\n' && bpr != '\n' ) wnext(cp,'\n');
  wnext(cp, 0);
  return ret;
}

void load(FILE *afp, FILE *bfp)
{
  int no = 0;
  int ins = 0, rep = 0;
  uint8_t inp[MX_STR];

  while( bgets(inp, sizeof(inp), afp) ) {
    ++no;
    uint8_t *bp = inp;
    string key = xlat(bp);
    if( bfp ) {
      if( !bgets(inp, sizeof(inp), bfp) ) {
        fprintf(stderr,"xlat file ended early\n");
        exit(1);
      }
      bp = inp;
    }
    else if( !is_sep(*bp++) ) {
      fprintf(stderr, "missing sep at line %d: %s", no, inp);
      exit(1);
    }
    string txt = xlat(bp);
    const char *val = (const char *)bp;
    uint8_t str[MX_STR];
    bool ok = chkfmt(no, (uint8_t*)key.c_str(), (uint8_t*)txt.c_str(), str);
      val = (const char*)str;
    Trans::iterator it = trans.lower_bound(key);
    if( it == trans.end() || it->first.compare(key) ) {
      trans.insert(it, Trans::value_type(key, tstring(val, ok)));
      ++ins;
    }
    else {
      it->second.assign(val);
      it->second.ok = ok;
      ++rep;
    }
  }
  fprintf(stderr,"***     ins %d, rep %d\n", ins, rep);
}

void scan_po(FILE *ifp, FILE *ofp)
{
  int no = 0;
  uint8_t ibfr[MX_STR], tbfr[MX_STR];

  while( bgets(ibfr, sizeof(ibfr), ifp) ) {
    if( !prefix_is(ibfr, "msgid ") ) {
      if( nocmts && ibfr[0] == '#' ) continue;
      no += bputs(ibfr, ofp);
      continue;
    }
    uint8_t str[MX_STR]; xlat2(&ibfr[6], str);
    string key((const char*)str);
    if( !bgets(tbfr, sizeof(tbfr), ifp) ) {
      fprintf(stderr, "file truncated line %d: %s", no, ibfr);
      exit(1);
    }
    no += bputs(ibfr, ofp);
   
    while( tbfr[0] == '"' ) {
      no += bputs(tbfr, ofp);
      xlat2(&tbfr[0], str);  key.append((const char*)str);
      if( !bgets(tbfr, sizeof(tbfr), ifp) ) {
        fprintf(stderr, "file truncated line %d: %s", no, ibfr);
        exit(1);
      }
    }
    if( !prefix_is(tbfr, "msgstr ") ) {
      fprintf(stderr, "file truncated line %d: %s", no, ibfr);
      exit(1);
    }

    if( !ofp ) {
      if( !key.size() ) continue;
      xlat3(key.c_str(), str);
      printf("%s\n", (char *)str);
      continue;
    }

    Trans::iterator it = trans.lower_bound(key);
    if( it == trans.end() || it->first.compare(key) ) {
      fprintf(stderr, "no trans line %d: %s\n", no, ibfr);
      xlat3(key.c_str(), &tbfr[7]);
      //no += bputs(tbfr, ofp);
      no += bputs((uint8_t*)"msgstr \"\"", ofp);
    }
    else if( 0 && !it->second.ok ) {
      fprintf(stderr, "bad fmt line %d: %s\n", no, ibfr);
      xlat3(it->first.c_str(), &tbfr[7]);
      no += bputs(tbfr, ofp);
      xlat3(it->second.c_str(), str);
      bput((uint8_t*)"#msgstr ", ofp);
      no += bputs(str, ofp);
    }
    else {
      xlat3(it->second.c_str(), &tbfr[7]);
      no += bputs(tbfr, ofp);
    }
  }
  if( ifp != stdin ) fclose(ifp);
}

void list_po(FILE *ifp, FILE *ofp, int xeqx = 0, int nnul = 0)
{
  int no = 0;
  int dup = 0, nul = 0;
  uint8_t ibfr[MX_STR], tbfr[MX_STR];

  while( bgets(ibfr, sizeof(ibfr), ifp) ) {
    ++no;
    if( !prefix_is(ibfr, "msgid ") ) continue;
    uint8_t str[MX_STR]; xlat2(&ibfr[6], str);
    string key((const char*)str);
    if( !bgets(tbfr, sizeof(tbfr), ifp) ) {
      fprintf(stderr, "file truncated line %d: %s", no, ibfr);
      exit(1);
    }
    ++no;
   
    while( tbfr[0] == '"' ) {
      xlat2(&tbfr[0], str);  key.append((const char*)str);
      if( !bgets(tbfr, sizeof(tbfr), ifp) ) {
        fprintf(stderr, "file truncated line %d: %s", no, ibfr);
        exit(1);
      }
      ++no;
    }
    if( !prefix_is(tbfr, "msgstr ") ) {
      fprintf(stderr, "file truncated line %d: %s", no, ibfr);
      exit(1);
    }

    xlat2(&tbfr[7], str);
    string txt((const char*)str);
   
    while( bgets(tbfr, sizeof(tbfr), ifp) && tbfr[0] == '"' ) {
      xlat2(&tbfr[0], str);  txt.append((const char*)str);
      ++no;
    }
    if( nnul && !txt.size() ) {
      ++nul;
      if( nnul > 0 ) continue;
    }
    else if( xeqx && !key.compare(txt) ) {
       ++dup;
       if( xeqx > 0 ) continue;
    }
    else if( nnul < 0 || xeqx < 0 ) continue;
    xlat4(key.c_str(), str);
    fprintf(ofp, "%s,", (char *)str);
    xlat4(txt.c_str(), str);
    fprintf(ofp, "%s\n", (char *)str);
  }
  fprintf(stderr, "*** dup %d, nul %d\n", dup, nul);
}

static void usage(const char *av0)
{
  printf("list csv    %s csv < data.csv > data.po\n",av0);
  printf("list po     %s po < data.po > data.csv\n",av0);
  printf("list po     %s dups < data.po\n",av0);
  printf("list po     %s nodups < data.po\n",av0);
  printf("get strings %s key  < xgettext.po\n",av0);
  printf("gen xlation %s xlat   xgettext.po xlat.csv\n",av0);
  printf("gen xlation %s xlat - text,xlat ... < xgettext.po\n",av0);
  exit(1);
}

int main(int ac, char **av)
{
  if( ac == 1 ) usage(av[0]);

  // if to rework google xlat output
  if( getenv("GOOG") ) goog = true;
  if( getenv("NOCMTS") ) nocmts = true;

  if( !strcmp(av[1],"csv") ) {  // test csv
    load(stdin, 0);
    for( Trans::iterator it = trans.begin(); it!=trans.end(); ++it ) {
      uint8_t str1[MX_STR];  xlat3(it->first.c_str(), str1);
      printf("msgid %s\n", (char *)str1);
      uint8_t str2[MX_STR];  xlat3(it->second.c_str(), str2);
      printf("msgstr %s\n\n", (char *)str2);
    }
    return 0;
  }

  if( !strcmp(av[1],"dups") ) {  // test po
    list_po(stdin, stdout, -1, -1);
    return 0;
  }

  if( !strcmp(av[1],"nodups") ) {  // test po
    list_po(stdin, stdout, 1, 1);
    return 0;
  }

  if( !strcmp(av[1],"po") ) {  // test po
    list_po(stdin, stdout);
    return 0;
  }

  if( !strcmp(av[1],"key") ) {
    scan_po(stdin, 0);
    return 0;
  }

  if( ac < 3 ) usage(av[0]);

  FILE *ifp = !strcmp(av[2],"-") ? stdin : fopen(av[2], "r");
  if( !ifp ) { perror(av[2]);  exit(1); }
 
//  if( ac < 4 ) usage(av[0]);

  if( strcmp(av[1],"xlat") ) {
    fprintf(stderr,"unkn cmd: %s\n", av[1]);
    return 1;
  }

  brkput = 1;
  for( int i=3; i<ac; ++i ) {  // create trans mapping
    fprintf(stderr,"*** load %s\n", av[i]);
    char fn[MX_STR*2];
    strncpy(fn, av[i], sizeof(fn));
    int k = 0;
    FILE *bfp = 0;
    // look for <filename> or <filename>,<filename>
    while( k<(int)sizeof(fn) && fn[k]!=0 && fn[k]!=',' ) ++k;
    if( k<(int)sizeof(fn) && fn[k]==',' ) {
      fn[k++] = 0;
      bfp = fopen(&fn[k], "r");
      if( !bfp ) { perror(&fn[k]);  exit(1); }
    }
    FILE *afp = fopen(&fn[0], "r");
    if( !afp ) { perror(&fn[0]);  exit(1); }
    load(afp, bfp);
    fclose(afp);
    if( bfp ) fclose(bfp);
  }

  scan_po(ifp, stdout);
  return 0;
}

