#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <list>

using namespace std;

#define MAX_LINE_SZ 256
#define expect(c) do { if( ch != c ) return -1; ++cp; } while(0)

class iline {
public:
  FILE *fp;  long pos;
  char line[MAX_LINE_SZ];
  int no;
  iline(FILE *f) : fp(f), pos(0), no(0) { line[0] = 0; }
  iline(iline &i) {
    fp = i.fp;  pos = i.pos;
    strcpy(line,i.line);  no = i.no;
  }
};

class ichar : public virtual iline {
public:
  int i, n;
  ichar(FILE *f) : iline(f) { i = n = 0; }
  ~ichar() {}
  ichar tws();
  char &operator *();
  char &operator [](int n) { return line[i+n]; }
  char &operator +=(int n) { i+=n; return operator*(); }
  char &operator -=(int n) { i-=n; return operator*(); }
  friend ichar &operator ++(ichar &t);
  friend char operator ++(ichar &t,int);
};

ichar &operator ++(ichar &t) { ++t.i; return t; }
char operator ++(ichar &t,int) { return t.line[t.i++]; }

char &ichar::operator *()
{
  if( i == n ) {
    int ch;  fseek(fp, pos, SEEK_SET);
    for( i=n=0; n<MAX_LINE_SZ; ) {
      if( (ch=fgetc(fp)) < 0 ) ch = 0;
      if( (line[n++]=ch) == '\n' || !ch ) break;
    }
    if( ch == '\n' ) ++no;
    pos = ftell(fp);
  }
  return line[i];
}

class tobj;
class iobj;
typedef list<string *> eobjs;

class dobj {
public:
  tobj *top;
  string *name;
  enum { ty_none, ty_bit, ty_boolean,
    ty_tinyint, ty_smallint, ty_mediumint, ty_decimal,
    ty_integer, ty_bigint, ty_real, ty_double, ty_float,
    ty_date, ty_time, ty_timestamp, ty_datetime, ty_year,
    ty_char, ty_varchar, ty_binary, ty_varbinary,
    ty_blob, ty_tinyblob, ty_mediumblob, ty_longblob,
    ty_text, ty_tinytext, ty_mediumtext, ty_longtext,
    ty_enum, ty_set, ty_media } ty;
  enum {
    var_types = (1<<ty_varchar) | (1<<ty_varbinary) | (1<<ty_blob) |
      (1<<ty_tinyblob) | (1<<ty_mediumblob) | (1<<ty_longblob) |
      (1<<ty_media),
    text_types = (1<<ty_text) |
      (1<<ty_tinytext) | (1<<ty_mediumtext) | (1<<ty_longtext),
    integer_types = (1<<ty_boolean) | (1<<ty_bit) | (1<<ty_binary) |
      (1<<ty_tinyint) | (1<<ty_smallint) | (1<<ty_decimal) |
      (1<<ty_mediumint) | (1<<ty_integer) | (1<<ty_bigint),
    double_types = (1<<ty_real) | (1<<ty_double) | (1<<ty_float),
  };
  int is_var() { return (var_types>>ty) & 1; }
  int is_integer() { return (integer_types>>ty) & 1; }
  int is_double() { return (double_types>>ty) & 1; }
  int length, zeroed, is_null, is_autoincr, is_binary, is_unsign, has_def;
  iobj *idx;
  eobjs eop;
  enum def_type { def_none, def_integer, def_double, def_string, def_null, };
  union { int i; double d; string *s; } def;
  dobj(tobj *tp) : top(tp) {
    ty = ty_none;  idx = 0;
    length = zeroed = is_null = is_autoincr = is_binary = is_unsign = has_def = -1;
  }
  ~dobj() {}
};

typedef list<dobj*> dobjs;

class tobj {
public:
  string *name;
  dobjs dop;
  dobj *dobj_of(const char *knm);

  tobj() : name(0) {}
  ~tobj() {};
};

list<tobj*> tables;

tobj *tobj_of(const char *tnm)
{
  list<tobj*>::iterator it = tables.begin();
  list<tobj*>::iterator eit = tables.end();
  while( it!=eit && *(*it)->name!=tnm ) ++it;
  return it == eit ? 0 : *it;
}

dobj *tobj::dobj_of(const char *knm)
{
    dobjs::iterator eid = dop.end();
    dobjs::iterator id = dop.begin();
    while( id!=eid && *(*id)->name!=knm ) ++id;
    return id == eid ? 0 : *id;
}

class iobj {
public:
  string *name;
  tobj *tbl;
  dobjs keys;
  short unique, primary;
  int is_dir;
  int is_direct();
  iobj() : name(0), tbl(0), unique(0), primary(0), is_dir(-1) {}
  ~iobj() {};
};

list<iobj*> indecies;

class robj {
public:
  string *name;
  enum { xref_no_action, xref_restrict, xref_cascade, xref_set_null, };
  int on_delete, on_update;
  list<string *> keys;
  robj() : on_delete(xref_no_action), on_update(xref_no_action) {}
  ~robj() {}
};


// trim white space
ichar &tws(ichar &c)
{
  ichar b = c;
  for(;;) {
    if( isspace(*b) ) { ++b; continue; }
    if( *b == '/' && b[1] == '*' ) {
      ++b;  do { ++b; } while( *b != '*' || b[1] != '/' );
      b += 2;  continue;
    }
    else if( *b == '-' && b[1] == '-' ) {
      ++b;  do { ++b; } while( *b != '\n' );
      ++b;  continue;
    }
    break;
  }
  c = b;
  return c;
}

// scan id
string *tid(ichar &c, string *sp=0)
{
  string *rp = 0;
  ichar b = tws(c);
  if( *b && (isalpha(*b) || *b == '_') ) {
    rp = !sp ? new string() : sp;
    rp->push_back(toupper(b++));
    while( *b && (isalpha(*b) || isdigit(*b) ||  *b == '_') ) {
      rp->push_back(tolower(b++));
    }
  }
  if( rp ) c = b;
  return rp;
}

// scan string '..' or ".."
int str_value(ichar &cp, string *&sp)
{
  ichar bp = cp;  sp = 0;
  int ch = *cp;
  if( ch != '\'' && ch != '"' ) return -1;
  int delim = ch;  ++cp;
  for( sp=new string(); (ch=*cp) && ch != delim; ++cp ) {
    if( ch == '\\' && !(ch=*++cp) ) break;
    sp->push_back(ch);
  }
  if( ch != delim ) {
    delete sp;  sp = 0;
    cp = bp;
    return -1;
  }
  ++cp;
  return 0;
}

// scan string '..' or "..", or number, or NULL
int def_value(ichar &cp, dobj *dp)
{
  ichar bp = cp;
  if( str_value(cp,dp->def.s) >= 0 ) {
    dp->has_def = dobj::def_string;
    return 0;
  }
  int ch = *tws(cp);
  if( isdigit(ch) || ch == '-' || ch == '+' ) {
    int sign = 0;
    if( ch == '-' ) sign = -1;
    if( ch == '+' ) sign = 1;
    if( sign ) ++cp; else sign = 1;
    int n = 0;
    while( (ch-='0') >= 0 && ch <= 9 ) {
      n = n*10 + ch;  ch = *++cp;
    }
    if( ch == '.' ) {
      double v = n;
      double frac = 1.;
      while( (ch-='0') >= 0 && ch <= 9 ) {
        v = v + ch/(frac*=10);  ch = *++cp;
      }
      dp->has_def = dobj::def_double;
      dp->def.d = v * sign;
    }
    else {
      dp->has_def = dobj::def_integer;
      dp->def.i = n * sign;
    }
    return 0;
  }
  string *id = tid(cp);
  if( id ) {
    if( *id == "Null" ) {
      dp->has_def = dobj::def_null;
      dp->def.s = 0;
      return 0;
    }
    delete id;
  }
  cp = bp;
  return -1;
}

// scan (number)
int dimen(ichar &cp, dobj *dp)
{
  int n = 0;
  int ch = *tws(cp);
  if( ch == '(' ) {
    ++cp;  ch = *tws(cp);
    while( (ch-='0') >= 0 && ch <= 9 ) {
      n = n*10 + ch;  ch = *++cp;
    }
    ch = *tws(cp);  expect(')');
    dp->length = n;
  }
  return 0;
}

// scan Unsigned or Signed, Zerofill
int numeric(ichar &cp, dobj *dp)
{
  ichar bp = cp;
  string *id = tid(cp);
  if( id && *id == "Unsigned" ) {
    dp->is_unsign = 1;
    bp = cp;  id = tid(cp);
  }
  else if( id && *id == "Signed" ) {
    dp->is_unsign = 0;
    bp = cp;  id = tid(cp);
  }
  if( id && *id == "Zerofill" ) {
    dp->zeroed = 1;
  }
  else
    cp = bp;
  return 0;
}

// (number) or numeric qualifier
int dimen0(ichar &cp, dobj *dp)
{
  return dimen(cp, dp) < 0 || numeric(cp, dp) < 0 ? -1 : 0;
}

// scan data type
int dtype(ichar &cp, dobj *dp)
{
  string *id = tid(cp);

  if( id ) {
    if( *id == "Bit" ) {
      dp->ty = dobj::ty_bit;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Tinyint" ) {
      dp->ty = dobj::ty_tinyint;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Smallint" ) {
      dp->ty = dobj::ty_smallint;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Mediumint" ) {
      dp->ty = dobj::ty_mediumint;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Int" || *id == "Integer" ) {
      dp->ty = dobj::ty_integer;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Bigint" ) {
      dp->ty = dobj::ty_bigint;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Real" ) {
      dp->ty = dobj::ty_real;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Double" ) {
      dp->ty = dobj::ty_double;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Float" ) {
      dp->ty = dobj::ty_float;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Decimal" ) {
      dp->ty = dobj::ty_decimal;
      if( dimen0(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Date" ) {
      dp->ty = dobj::ty_date;
      return 0;
    }
    else if( *id == "Time" ) {
      dp->ty = dobj::ty_time;
      return 0;
    }
    else if( *id == "Timestamp" ) {
      dp->ty = dobj::ty_timestamp;
      return 0;
    }
    else if( *id == "Datetime" ) {
      dp->ty = dobj::ty_datetime;
      return 0;
    }
    else if( *id == "Year" ) {
      dp->ty = dobj::ty_year;
      return 0;
    }

    if( *id == "Char" ) {
      dp->ty = dobj::ty_char;
      if( dimen(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Varchar" ) {
      dp->ty = dobj::ty_varchar;
      if( dimen(cp, dp) < 0 ) return -1;
      if( (id=tid(cp)) != 0 ) {
        ichar bp = cp;
        if( *id == "Binary" ) {
          dp->is_binary = 1;
        }
        else
          cp = bp;
      }
      return 0;
    }

    if( *id == "Binary" ) {
      dp->ty = dobj::ty_binary;
      if( dimen(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Varbinary" ) {
      dp->ty = dobj::ty_varbinary;
      if( dimen(cp, dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Tinyblob" ) {
      dp->ty = dobj::ty_tinyblob;
      return 0;
    }

    if( *id == "Blob" ) {
      dp->ty = dobj::ty_blob;
      return 0;
    }

    if( *id == "Mediumblob" ) {
      dp->is_unsign = 1;
      dp->ty = dobj::ty_mediumblob;
      return 0;
    }

    if( *id == "Longblob" ) {
      dp->ty = dobj::ty_longblob;
      return 0;
    }

    if( *id == "Tinytext" ) {
      dp->ty = dobj::ty_tinytext;
      return 0;
    }

    if( *id == "Text" ) {
      dp->ty = dobj::ty_text;
      return 0;
    }

    if( *id == "Mediumtext" ) {
      dp->ty = dobj::ty_mediumtext;
      return 0;
    }

    if( *id == "Longtext" ) {
      dp->ty = dobj::ty_longtext;
      return 0;
    }

    if( *id == "Bool" || *id == "Boolean" ) {
      dp->ty = dobj::ty_boolean;
      return 0;
    }

    if( *id == "Enum" ) {
      dp->ty = dobj::ty_enum;
      int ch = *tws(cp);  expect('(');
      if( str_value(tws(cp),id) < 0 ) return -1;
      dp->eop.push_back(id);
      while( *tws(cp) == ',' ) {
        if( str_value(tws(++cp),id) < 0 ) return -1;
        dp->eop.push_back(id);
      }
      ch = *tws(cp);  expect(')');
      return 0;
    }

    if( *id == "Media" ) {
      dp->ty = dobj::ty_media;
      return 0;
    }
  }

  return -1;
}

// scan reference
int xrefer(ichar &cp, robj *rp)
{
  string *id = tid(cp);
  if( !id ) return -1;
  rp->name = id;
  int ch = *tws(cp);  expect('(');
  if( !(id=tid(cp)) ) return -1;
  rp->keys.push_back(id);
  while( *tws(cp) == ',' ) {
    if( !(id=tid(++cp)) ) return -1;
    rp->keys.push_back(id);
  }
  ch = *tws(cp);  expect(')');
  if( !(id=tid(cp)) ) return -1;
  if( *id != "On" ) return -1;
  if( !(id=tid(cp)) ) return -1;
  int *pp = 0;
  if( *id == "Delete" ) {
    pp = &rp->on_delete;
  }
  else if( *id == "Update" ) {
    pp = &rp->on_update;
  }
  if( !pp ) return -1;
  if( !(id=tid(cp)) ) return -1;
  if( *id == "Restrict" ) {
    rp->on_delete = robj::xref_restrict;
    return 0;
  }
  if( *id == "Cascade" ) {
    rp->on_delete = robj::xref_cascade;
    return 0;
  }
  if( *id == "Set" ) {
    if( !(id=tid(cp)) ) return -1;
    if( *id != "Null" ) return -1;
    rp->on_delete = robj::xref_set_null;
    return 0;
  }
  if( *id == "No" ) {
    if( !(id=tid(cp)) ) return -1;
    if( *id != "Action" ) return -1;
    rp->on_delete = robj::xref_no_action;
    return 0;
  }
  return -1;
}       

// scan datatype extension
int xtype(ichar &cp, dobj *dp)
{
  string *id = tid(cp);
  if( id ) {
    if( *id == "Not" ) {
      if( *tid(cp) != "Null" ) return -1;
      dp->is_null = 0;
      return 0;
    }

    if( *id == "Null" ) {
      dp->is_null = 1;
      return 0;
    }

    if( *id == "Default" ) {
      if( def_value(tws(cp),dp) < 0 ) return -1;
      return 0;
    }

    if( *id == "Auto_increment" ) {
      dp->is_autoincr = 1;
      if( dp->idx ) dp->idx->unique = 1;
      return 0;
    }

    if( *id == "Unique" || *id == "Primary" ) {
      int unique = dp->is_autoincr > 0 ? 1 : 0;
      int primary = 0;
      if( *id == "Unique" ) {
        unique = 1;  id = tid(cp);
      }
      if( *id == "Primary" ) primary = 1;
      ichar bp = cp;
      if( *tid(cp) != "Key" ) cp = bp;
      iobj *idx = new iobj();
      dp->idx = idx;
      idx->name = new string(*dp->name);
      idx->name->push_back('_');
      idx->tbl = dp->top;
      idx->unique = unique;
      idx->primary = primary;
      idx->keys.push_back(dp);
      indecies.push_back(idx);
      return 0;
    }

#if 0
    if( *id == "Comment" ) {
      return 0;
    }

    if( *id == "Column_format" ) {
      return 0;
    }

    if( *id == "Storage" ) {
      return 0;
    }
#endif

    if( *id == "References" ) {
      ichar bp = cp;
      robj *rop = new robj();
      if ( xrefer(cp, rop) < 0 ) {
        cp = bp;
        return -1;
      }
      return 0;
    }
  }
  return -1;
}

// scan table member line
int member(ichar &cp, dobj *dp)
{
  dp->name = tid(cp);
  if( dtype(cp,dp) < 0 ) return -1;

  for( int ch=*tws(cp); (ch=*tws(cp)) && isalpha(ch); ) {
    if( xtype(cp,dp) < 0 ) return -1;
  }

  return 0;
}

// scan create table line
int table(ichar &cp, tobj *top)
{
  string *id = tid(cp);
  if( !id ) return -1;
  top->name = id;
  int ch = *tws(cp);  expect('(');
  for(;;) {
    dobj *dp = new dobj(top);
    if( member(cp,dp) < 0 ) {
      printf("  err in %s/%s at %d\n",id->c_str(),dp->name->c_str(),cp.no);
      delete dp;
      return -1;
    }
    top->dop.push_back(dp);
    if( (ch=*tws(cp)) == ')' ) break;
    expect(',');
  }
  expect(')');
  while( (id=tid(cp)) ) {
    ch = *tws(cp);  expect('=');
    printf("  skipping %s at %d\n",id->c_str(),cp.no);
    if( isdigit(*tws(cp)) ) while( isdigit(*++cp) );
    else if( !tid(cp) ) return -1;
  }
 ch = *tws(cp);  expect(';');
  return 0;
}

int iobj::is_direct()
{
  if( is_dir < 0 ) {
    is_dir = 1;
    dobjs::iterator ekey = keys.end();
    dobjs::iterator key = keys.begin();
    for( int i=0; key!=ekey; ++i ) {
      dobj *dp = *key++;
      if( dp->is_integer() ) continue;
      if( dp->is_double() ) continue;
      is_dir = 0; break;
    }
  }
  return is_dir;
}

// scan (number)
int keylen(ichar &cp, iobj *ip)
{
  int n = 0;
  int ch = *tws(cp);
  if( ch == '(' ) {
    ++cp;  ch = *tws(cp);
    while( (ch-='0') >= 0 && ch <= 9 ) {
      n = n*10 + ch;  ch = *++cp;
    }
    ch = *tws(cp);  expect(')');
  }
  return 0;
}

// scan create index line
int index(ichar &cp, iobj *iop)
{
  string *id = tid(cp);
  if( !id ) return -1;
  iop->name = id;
  if( !(id=tid(cp)) ) return -1;
  if( *id != "On" ) return -1;
  if( !(id=tid(cp)) ) return -1;
  tobj *tp = tobj_of(id->c_str());
  if( !tp ) {
    printf("index err On:%s at %d\n",id->c_str(),cp.no);
    return -1;
  }
  iop->tbl = tp;
  int ch = *tws(cp);  expect('(');
  if( !(id=tid(cp)) ) return -1;
  dobj *dp = tp->dobj_of(id->c_str());
  if( !dp ) {
    printf("index err Key:%s/%s at %d\n",tp->name->c_str(),id->c_str(),cp.no);
    return -1;
  }
  iop->keys.push_back(dp);
  if( keylen(cp,iop) < 0 ) return -1;
  while( *tws(cp) == ',' ) {
    if( !(id=tid(++cp)) ) return -1;
    dp = tp->dobj_of(id->c_str());
    if( !dp ) {
      printf("index err Key:%s/%s at %d\n",tp->name->c_str(),id->c_str(),cp.no);
      return -1;
    }
    iop->keys.push_back(dp);
    if( keylen(cp,iop) < 0 ) return -1;
  }
  ch = *tws(cp);  expect(')');
  ch = *tws(cp);  expect(';');
  return 0;
}

// process create line
int create(ichar &cp)
{
  int online = -1; (void) online; // avoid unused warn
  enum { none, unique, fulltext, spatial } modifier = none;

  string *tp = tid(cp);
  if( *tp == "Table" ) {
    printf("Create Table at %d\n",cp.no);
    tobj *top = new tobj();
    if( table(cp, top) < 0 ) {
      delete top;
      return -1;
    }
    tables.push_back(top);
    return 0;
  }
  if( *tp == "Online" ) {
    online = 1;
    tp = tid(cp);
  }
  else if( *tp == "Offine" ) {
    online = 0;
    tp = tid(cp);
  }
  if( *tp == "Unique" ) {
    modifier = unique;
    tp = tid(cp);
  }
  else if( *tp == "Fulltext" ) {
    modifier = fulltext;
    tp = tid(cp);
  }
  else if( *tp == "Spatial" ) {
    modifier = spatial;
    tp = tid(cp);
  }
  if( *tp == "Index" ) {
    printf("Create index at %d\n",cp.no);
    iobj *iop = new iobj();
    if( index(cp, iop) < 0 ) {
      delete iop;
      printf("== FAILED!\n");
      return -1;
    }
    if( modifier == unique ) iop->unique = 1;
    indecies.push_back(iop);
    return 0;
  }
  fprintf(stderr,"unknown keyword - %s\n", tp->c_str());
  return -1;
}

void varg_dobj(FILE *sp, dobj *dp, const char *ty, int is_unsign=-1)
{
  const char *tnm = dp->top->name->c_str();
  const char *nm = dp->name->c_str();
  fprintf(sp,"const %sObj::t_%s &%s", tnm, nm, nm);
}

void arg_dobj(FILE *sp, dobj *dp, const char *ty, int is_unsign=-1)
{
  if( is_unsign < 0 ) is_unsign = dp->is_unsign;
  if( is_unsign > 0 ) fprintf(sp, "unsigned ");
  fprintf(sp,"%s ", ty);
  if( dp->length > 0 ) fprintf(sp, "*");
  fprintf(sp,"%s", dp->name->c_str());
}

void arg_dobj(FILE *sp, dobj *dp)
{
  switch( dp->ty ) {
  case dobj::ty_boolean:
  case dobj::ty_bit:
  case dobj::ty_binary:   arg_dobj(sp, dp, "char", 1); break;
  case dobj::ty_char:
  case dobj::ty_tinyint:  arg_dobj(sp, dp, "char"); break;
  case dobj::ty_enum:
  case dobj::ty_smallint: arg_dobj(sp, dp, "short"); break;
  case dobj::ty_decimal:
  case dobj::ty_mediumint:
  case dobj::ty_integer:  arg_dobj(sp, dp, "int"); break;
  case dobj::ty_bigint:   arg_dobj(sp, dp, "long"); break;
  case dobj::ty_real:
  case dobj::ty_double:   arg_dobj(sp, dp, "double", 0); break;
  case dobj::ty_float:    arg_dobj(sp, dp, "float", 0); break;
  case dobj::ty_date:
  case dobj::ty_time:
  case dobj::ty_timestamp:
  case dobj::ty_datetime:
  case dobj::ty_year:     varg_dobj(sp, dp, "char",0); break;
  case dobj::ty_varchar:
    if( dp->is_binary ) { varg_dobj(sp, dp, "char",1); break; }
  case dobj::ty_tinytext:
  case dobj::ty_text:
  case dobj::ty_mediumtext:
  case dobj::ty_longtext: varg_dobj(sp, dp, "char",0); break;
  case dobj::ty_varbinary:
  case dobj::ty_tinyblob:
  case dobj::ty_blob:
  case dobj::ty_mediumblob:
  case dobj::ty_longblob:
  case dobj::ty_media:    varg_dobj(sp, dp, "char",1); break;
  default:
    fprintf(sp,"unimplemented %s,", dp->name->c_str());
    break;
  }
}

void put_targs(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    if( !dp || dp->ty == dobj::ty_none ) {
      fprintf(stderr," %s member %s error: ty_none\n",
        tp->name->c_str(),dp->name->c_str());
      continue;
    }
    fprintf(sp,"        ");
    arg_dobj(sp, dp);
    if( key == ekey ) break;
    fprintf(sp,",\n");
  }
}

const char *typ_dobj(dobj *dp)
{
  const char *cp;
  switch( dp->ty ) {
  case dobj::ty_boolean:
  case dobj::ty_bit:
  case dobj::ty_binary:    cp = "unsigned char"; break;
  case dobj::ty_char:
  case dobj::ty_tinyint:   cp = dp->is_unsign > 0 ? "unsigned char" : "char"; break;
  case dobj::ty_enum:
  case dobj::ty_smallint:  cp = dp->is_unsign > 0 ? "unsigned short" : "short"; break;
  case dobj::ty_decimal:
  case dobj::ty_mediumint:
  case dobj::ty_integer:   cp = dp->is_unsign > 0 ? "unsigned int" : "int"; break;
  case dobj::ty_bigint:    cp = dp->is_unsign > 0 ? "unsigned long" : "long"; break;
  case dobj::ty_real:
  case dobj::ty_double:    cp = "double"; break;
  case dobj::ty_float:     cp = "float"; break;
  case dobj::ty_date:
  case dobj::ty_time:
  case dobj::ty_timestamp:
  case dobj::ty_datetime:
  case dobj::ty_year:      cp = "char"; break;
  case dobj::ty_varchar:
    if( dp->is_binary ) {  cp = "unsigned char"; break; }
  case dobj::ty_tinytext:
  case dobj::ty_text:
  case dobj::ty_mediumtext:
  case dobj::ty_longtext:  cp = "char"; break;
  case dobj::ty_varbinary:
  case dobj::ty_tinyblob:
  case dobj::ty_blob:
  case dobj::ty_mediumblob:
  case dobj::ty_longblob:
  case dobj::ty_media:     cp = "unsigned char"; break;
  default:                 cp = "unimplemented"; break;
  }
  return cp;
}

void put_keysz(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();
  const char *tnm = tp->name->c_str();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp," +sizeof(%sObj::t_%s)", tnm, nm);
  }
  if( ip->unique <= 0 ) fprintf(sp,"+sizeof(int)");
}

void put_keys(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey = ip->keys.end();
  dobjs::iterator key = ip->keys.begin();
  const char *tnm = tp->name->c_str();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp,"    %sObj::t_%s v_%s;\n", tnm, nm, nm);
  }
  if( ip->unique <= 0 ) fprintf(sp,"    int v_id;\n");
}

void put_init(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey = ip->keys.end();
  dobjs::iterator key = ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp,",\n      v_%s(%s)", nm, nm);
  }
}

const char *put_cmpr(dobj *dp)
{
  const char *cp;
  switch( dp->ty ) {
  case dobj::ty_boolean:
  case dobj::ty_bit:
  case dobj::ty_binary:    cp = "uchar"; break;
  case dobj::ty_char:
  case dobj::ty_tinyint:   cp = dp->is_unsign > 0 ? "uchar" : "char"; break;
  case dobj::ty_enum:
  case dobj::ty_smallint:  cp = dp->is_unsign > 0 ? "ushort" : "short"; break;
  case dobj::ty_decimal:
  case dobj::ty_mediumint:
  case dobj::ty_integer:   cp = dp->is_unsign > 0 ? "uint" : "int"; break;
  case dobj::ty_bigint:    cp = dp->is_unsign > 0 ? "ulong" : "long"; break;
  case dobj::ty_real:
  case dobj::ty_double:    cp = "double"; break;
  case dobj::ty_float:     cp = "float"; break;
  case dobj::ty_date:
  case dobj::ty_time:
  case dobj::ty_timestamp:
  case dobj::ty_datetime:
  case dobj::ty_year:      cp = "char"; break;
  case dobj::ty_varchar:
    if( dp->is_binary ) {  cp = "uchar"; break; }
  case dobj::ty_tinytext:
  case dobj::ty_text:
  case dobj::ty_mediumtext:
  case dobj::ty_longtext:  cp = "char"; break;
  case dobj::ty_varbinary:
  case dobj::ty_tinyblob:
  case dobj::ty_blob:
  case dobj::ty_mediumblob:
  case dobj::ty_longblob:  cp = "uchar"; break;
  case dobj::ty_media:     cp = "media"; break;
  default:                 cp = "unimplemented"; break;
  }
  return cp;
}

void put_rkey(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp,"  if( bp ) memcpy(cp, kloc._%s(), kloc.size_%s());\n", nm, nm);
    fprintf(sp,"  cp += kloc.size_%s();\n", nm);
  }
  if( ip->unique <= 0 ) {
    fprintf(sp,"  if( bp ) memcpy(cp, kloc._id(), kloc._id_size());\n");
    fprintf(sp,"  cp += kloc._id_size();\n");
  }
}

void dir_rcmpr(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    fprintf(sp,"  { %s vv", typ_dobj(dp));
    if( dp->length > 0 ) fprintf(sp, "[%d]", dp->length);
    fprintf(sp,"; memcpy(&vv,b,sizeof(vv)); b += sizeof(vv);\n");
    fprintf(sp,"    int v = cmpr_%s", put_cmpr(dp));
    const char *nm = dp->name->c_str();
    fprintf(sp,"( kloc._%s(), kloc->v_%s.size(), &vv", nm, nm);
    if( dp->length > 0 ) fprintf(sp, "[0]");
    fprintf(sp,", sizeof(vv));\n");
    fprintf(sp,"    if( v != 0 ) return v; }\n");
  }
  if( ip->unique <= 0 ) {
    fprintf(sp,"  int vid; memcpy(&vid,b,sizeof(vid)); b += sizeof(vid);\n");
    fprintf(sp,"  int v = cmpr_int(kloc._id(), kloc._id_size(), &vid, sizeof(vid));\n");
    fprintf(sp,"  if( v != 0 ) return v;\n");
  }
}

void ind_rcmpr(FILE *sp, tobj *tp, iobj *ip)
{
  fprintf(sp,"  int b_id; memcpy(&b_id,b,sizeof(b_id));\n");
  fprintf(sp,"  if( kloc->id == b_id ) return 0;\n");
  fprintf(sp,"  %sLoc vloc(kloc.entity);\n", tp->name->c_str());
  fprintf(sp,"  if( vloc.FindId(b_id) )\n");
  fprintf(sp,"    kloc.err_(Db::errCorrupt);\n");
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp,"  { int v = cmpr_%s", put_cmpr(dp));
    fprintf(sp,"( kloc._%s(), kloc->v_%s.size(),\n", nm, nm);
    fprintf(sp,"                   vloc._%s(), vloc->v_%s.size());\n", nm, nm);
    fprintf(sp,"    if( v != 0 ) return v; }\n");
  }
  if( ip->unique <= 0 ) {
    fprintf(sp,"  { int v = cmpr_int(kloc._id(), kloc._id_size(),\n");
    fprintf(sp,"               vloc._id(), vloc._id_size());\n");
    fprintf(sp,"    if( v != 0 ) return v; }\n");
  }
}

void dir_icmpr(FILE *sp, tobj *tp, iobj *ip)
{
  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    fprintf(sp,"  { %s vv", typ_dobj(dp));
    if( dp->length > 0 ) fprintf(sp, "[%d]", dp->length);
    fprintf(sp,"; memcpy(&vv,b,sizeof(vv)); b += sizeof(vv);\n");
    fprintf(sp,"    int v = cmpr_%s", put_cmpr(dp));
    const char *nm = dp->name->c_str();
    fprintf(sp,"( kp->v_%s.addr(), kp->v_%s.size(), &vv", nm, nm);
    if( dp->length > 0 ) fprintf(sp, "[0]");
    fprintf(sp,", sizeof(vv));\n");
    fprintf(sp,"    if( v != 0 ) return v; }\n");
  }
  if( ip->unique <= 0 ) {
    fprintf(sp,"  if( kp->v_id >= 0 ) {\n");
    fprintf(sp,"    int vid; memcpy(&vid,b,sizeof(vid)); b += sizeof(vid);\n");
    fprintf(sp,"    int v = cmpr_int(&kp->v_id, sizeof(kp->v_id), &vid, sizeof(vid));\n");
    fprintf(sp,"    if( v != 0 ) return v;\n");
    fprintf(sp,"  }\n");
  }
}

void ind_icmpr(FILE *sp, tobj *tp, iobj *ip)
{
  fprintf(sp,"  int b_id;  memcpy(&b_id,b,sizeof(b_id)); b += sizeof(b_id);\n");
  if( ip->unique <= 0 ) fprintf(sp,"  if( kp->v_id == b_id ) return 0;\n");
  fprintf(sp,"  %sLoc vloc(kp->loc.entity);\n", tp->name->c_str());
  fprintf(sp,"  if( vloc.FindId(b_id) )\n");
  fprintf(sp,"    vloc.err_(Db::errCorrupt);\n");

  dobjs::iterator ekey=ip->keys.end();
  dobjs::iterator key=ip->keys.begin();

  while( key != ekey ) {
    dobj *dp = *key++;
    const char *nm = dp->name->c_str();
    fprintf(sp,"  { int v = cmpr_%s", put_cmpr(dp));
    fprintf(sp,"( kp->v_%s.addr(), kp->v_%s.size(),\n", nm, nm);
    fprintf(sp,"                  vloc._%s(), vloc->v_%s.size());\n", nm, nm);
    fprintf(sp,"    if( v != 0 ) return v; }\n");
  }
  if( ip->unique <= 0 ) {
    fprintf(sp,"  if( kp->v_id >= 0 ) {\n");
    fprintf(sp,"    int v = cmpr_int(&kp->v_id, sizeof(kp->v_id),\n");
    fprintf(sp,"                 vloc._id(), vloc._id_size());\n");
    fprintf(sp,"    if( v != 0 ) return v;\n");
    fprintf(sp,"  }\n");
  }
}

void put_dobj(FILE *sp, dobj *dp, const char *dty, int n)
{
  fprintf(sp,"  array_%s(char,%s,%d);\n", dty, dp->name->c_str(), n);
}

void put_dobj(FILE *sp, dobj *dp, const char *dty, const char *ty, int is_unsign=-1)
{
  fprintf(sp,"  %s_%s(", dp->length > 0 ? "array" : "basic", dty);
  if( is_unsign < 0 ) is_unsign = dp->is_unsign;
  if( is_unsign > 0 ) fprintf(sp, "unsigned ");
  fprintf(sp,"%s,%s", ty, dp->name->c_str());
  if( dp->length > 0 ) fprintf(sp, ",%d", dp->length);
  fprintf(sp, ");\n");
}

void put_decl(FILE *sp, dobj *dp, const char *dty)
{
  switch( dp->ty ) {
  case dobj::ty_boolean:
  case dobj::ty_bit:
  case dobj::ty_binary:   put_dobj(sp, dp, dty, "char", 1); break;
  case dobj::ty_char:
  case dobj::ty_tinyint:  put_dobj(sp, dp, dty, "char"); break;
  case dobj::ty_enum:
  case dobj::ty_smallint: put_dobj(sp, dp, dty, "short"); break;
  case dobj::ty_decimal:
  case dobj::ty_mediumint:
  case dobj::ty_integer:  put_dobj(sp, dp, dty, "int"); break;
  case dobj::ty_bigint:   put_dobj(sp, dp, dty, "long"); break;
  case dobj::ty_real:
  case dobj::ty_double:   put_dobj(sp, dp, dty, "double", 0); break;
  case dobj::ty_float:    put_dobj(sp, dp, dty, "float", 0); break;
  case dobj::ty_date:     put_dobj(sp, dp, dty, 8); break;
  case dobj::ty_time:     put_dobj(sp, dp, dty, 6); break;
  case dobj::ty_timestamp:
  case dobj::ty_datetime: put_dobj(sp, dp, dty, 14); break;
  case dobj::ty_year:     put_dobj(sp, dp, dty, 4); break;
  case dobj::ty_text:
  case dobj::ty_tinytext:
  case dobj::ty_mediumtext:
  case dobj::ty_longtext:
    fprintf(sp,"  sarray_%s(char,%s);\n", dty, dp->name->c_str());
    break;
  case dobj::ty_varchar:
    if( !dp->is_binary ) {
      fprintf(sp,"  varray_%s(char,%s);\n", dty, dp->name->c_str());
      break;
    }
  case dobj::ty_varbinary:
  case dobj::ty_blob:
  case dobj::ty_tinyblob:
  case dobj::ty_mediumblob:
  case dobj::ty_longblob:
  case dobj::ty_media:
    fprintf(sp,"  varray_%s(unsigned char,%s);\n", dty, dp->name->c_str());
    dp->is_unsign = 1;
    break;
  default:
    fprintf(sp," %s %s unimplemented;\n", dty, dp->name->c_str());
    break;
  }
}


int main(int ac, char **av)
{
  const char *tdb = ac > 1 ? av[1] : "theDb";
  const char *sfn = ac > 2 ? av[2] : "./s";

  setbuf(stdout,0);
  setbuf(stderr,0);
  ichar cp(stdin);

  for( string *sp=tid(cp); sp!=0; sp=tid(cp) ) {
    if( *sp == "Create" ) create(cp);
  }

  printf(" %d Tables\n",(int)tables.size());
  printf(" %d Indecies\n",(int)indecies.size());

  char fn[512];  sprintf(fn,"%s.h",sfn);
  FILE *sp = fopen(fn,"w");

  // get basename for sfn.h, _SFN_H_
  const char *bp = 0;
  for( const char *ap=sfn; *ap; ++ap )
    if( *ap == '/' ) bp = ap+1;
  if( !bp ) bp = sfn;
  char ups[512], *up = ups;  *up++ = '_';
  for( const char *cp=bp; *cp; ++cp ) *up++ = toupper(*cp);
  *up++ = '_';  *up++ = 'H';  *up++ = '_';  *up = 0;

  fprintf(sp,"#ifndef %s\n",ups);
  fprintf(sp,"#define %s\n",ups);
  fprintf(sp,"#include <cstdio>\n");
  fprintf(sp,"#include <stdlib.h>\n");
  fprintf(sp,"#include <unistd.h>\n");
  fprintf(sp,"#include <fcntl.h>\n");
  fprintf(sp,"#include <errno.h>\n");
  fprintf(sp,"\n");
  fprintf(sp,"#include \"tdb.h\"\n");
  fprintf(sp,"\n");
  fprintf(sp,"\n");

  list<tobj*>::iterator sit = tables.begin();
  list<tobj*>::iterator eit = tables.end();
  list<tobj*>::iterator it;

  list<iobj*>::iterator sidx = indecies.begin();
  list<iobj*>::iterator eidx = indecies.end();
  list<iobj*>::iterator idx;

  for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      ip->is_dir = ip->is_direct();
  }

  int i=0;
  for( it=sit ; it!=eit; ++i, ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    printf(" %2d. %s (",i,tnm);
    fprintf(sp,"// %s\n",tnm);
    fprintf(sp,"DbObj(%s)\n",tnm);

    list<dobj*>::iterator sid = tp->dop.begin();
    list<dobj*>::iterator eid = tp->dop.end();
    list<dobj*>::iterator id;

    // output table member declarations
    for( id=sid ; id!=eid; ++id ) {
      dobj *dp = *id;
      printf(" %s",dp->name->c_str());
      if( dp->ty == dobj::ty_none ) {
        fprintf(stderr," %s member %s error: ty_none\n",
          tnm,dp->name->c_str());
        continue;
      }
      put_decl(sp,dp,"def");
    }
    printf(" )\n");

    fprintf(sp,"};\n");
    fprintf(sp,"\n");

    int j = 0, n = 0;
    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      ++n;
      printf("   %2d.%d%c %s on %s(",i,j++,
        ip->is_dir>0 ? '=' : '.',
        ip->name->c_str(),tp->name->c_str());
      dobjs::iterator ekey=ip->keys.end();
      dobjs::iterator key=ip->keys.begin();

      while( key != ekey ) {
        dobj *dp = *key++;
        printf(" %s", dp->name->c_str());
      }
      printf(" )\n");
    }
    if( n > 0 ) printf("\n");

    // output table member accessors
    fprintf(sp,"DbLoc(%s)\n",tnm);
    for( id=sid ; id!=eid; ++id ) {
      dobj *dp = *id;
      if( dp->ty == dobj::ty_none ) {
        fprintf(stderr," %s member %s error: ty_none\n",
          tnm,dp->name->c_str());
        continue;
      }
      put_decl(sp, dp, "ref");
    }

    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *knm = ip->name->c_str();
      fprintf(sp,"\n");
      fprintf(sp,"  class key_%s { public:\n", knm);
      fprintf(sp,"    static const int size() { return 0");
      put_keysz(sp, *it, ip);
      fprintf(sp,"; }\n");
      fprintf(sp,"  };\n");
      fprintf(sp,"\n");
      fprintf(sp,"  class ikey_%s : public Db::iKey { public:\n", knm);
      put_keys(sp, *it, ip);
      fprintf(sp,"    static int icmpr(char *a, char *b);\n");
      // key constructors
      fprintf(sp,"    ikey_%s(ObjectLoc &loc,\n", knm);
      put_targs(sp, *it, ip);
      if( ip->unique <= 0 ) fprintf(sp,", int id=-1");
      fprintf(sp,")\n    : iKey(\"%s\",loc,icmpr)", knm);
      put_init(sp, *it, ip);
      if( ip->unique <= 0 ) fprintf(sp,",\n      v_id(id)");
      fprintf(sp," {}\n");
      fprintf(sp,"  };\n");
      fprintf(sp,"  class rkey_%s : public Db::rKey { public:\n", knm);
      fprintf(sp,"    static int rcmpr(char *a, char *b);\n");
      fprintf(sp,"    rkey_%s(ObjectLoc &loc) : rKey(\"%s\",loc,rcmpr) {}\n", knm, knm);
      fprintf(sp,"    int wr_key(char *cp=0);\n");
      fprintf(sp,"  };\n");
    }
    fprintf(sp,"\n");
    fprintf(sp,"  int Allocate();\n");
    fprintf(sp,"  int Construct();\n");
    fprintf(sp,"  int Destruct();\n");
    fprintf(sp,"  void Deallocate();\n");
#ifdef COPY
    fprintf(sp,"  int Copy(ObjectLoc &that);\n");
#endif
    fprintf(sp,"};\n");
  }
  fprintf(sp,"\n");

  fprintf(sp,"\n");
  fprintf(sp,"class %s : public Db {\n",tdb);
  fprintf(sp,"  int dfd, dkey, no_atime;\n");
  fprintf(sp,"  int db_create();\n");
  fprintf(sp,"  int db_open();\n");
  fprintf(sp,"  int db_access();\n");
  fprintf(sp,"public:\n");
  fprintf(sp,"  Objects objects;\n");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,"  Entity %s;  %sLoc %c%s;\n", tnm, tnm, tolower(tnm[0]), &tnm[1]);
  }
  fprintf(sp,"\n");
  fprintf(sp,"  int create(const char *dfn);\n");
  fprintf(sp,"  int open(const char *dfn, int key=-1);\n");
  fprintf(sp,"  int access(const char *dfn, int key=-1, int rw=0);\n");
  fprintf(sp,"  void close();\n");
  fprintf(sp,"  int attach(int rw=0) { return Db::attach(rw); }\n");
  fprintf(sp,"  int detach() { return Db::detach(); }\n");
  fprintf(sp,"\n");
  fprintf(sp,"  %s();\n",tdb);
  fprintf(sp,"  ~%s() { finit(objects); }\n",tdb);
  fprintf(sp,"};\n");
  fprintf(sp,"\n");
  fprintf(sp,"#endif\n");
  fclose(sp);

  sprintf(fn,"%s.C",sfn);
  sp = fopen(fn,"w");
  fprintf(sp,"#include \"%s.h\"\n",bp);
  fprintf(sp,"\n");

  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char * tnm = tp->name->c_str();

    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *knm = ip->name->c_str();
      fprintf(sp,"\n");
      fprintf(sp,"int %sLoc::ikey_%s::\n", tnm, knm);
      fprintf(sp,"icmpr(char *a, char *b)\n");
      fprintf(sp,"{\n");
      fprintf(sp,"  ikey_%s *kp = (ikey_%s *)a;\n", knm, knm);
      (ip->is_dir>0 ? dir_icmpr : ind_icmpr)(sp, tp, ip);
      fprintf(sp,"  return 0;\n");
      fprintf(sp,"}\n");
      fprintf(sp,"int %sLoc::rkey_%s::\n", tnm, knm);
      fprintf(sp,"rcmpr(char *a, char *b)\n");
      fprintf(sp,"{\n");
      fprintf(sp,"  rkey_%s *kp = (rkey_%s *)a;\n", knm, knm);
      fprintf(sp,"  %sLoc &kloc = (%sLoc&)kp->loc;\n", tnm, tnm);
      (ip->is_dir>0 ? dir_rcmpr : ind_rcmpr)(sp, tp, ip);
      fprintf(sp,"  return 0;\n");
      fprintf(sp,"}\n");
      fprintf(sp,"int %sLoc::rkey_%s::\n", tnm, knm);
      fprintf(sp,"wr_key(char *bp)\n");
      fprintf(sp,"{\n");
      fprintf(sp,"  char *cp = bp;\n");
      fprintf(sp,"  %sLoc &kloc = (%sLoc&)loc;\n", tnm, tnm);
      put_rkey(sp, tp, ip);
      fprintf(sp,"  return cp-bp;\n");
      fprintf(sp,"}\n");
    }
    fprintf(sp,"\n");
    fprintf(sp,"int %sLoc::Allocate()\n", tnm);
    fprintf(sp,"{\n");
    fprintf(sp,"  if_err( allocate() );\n");
    fprintf(sp,"  if( !addr_wr() ) return err_(Db::errNoMemory);\n");

    int n = 0;
    list<dobj*>::iterator sid = tp->dop.begin();
    list<dobj*>::iterator eid = tp->dop.end();
    list<dobj*>::iterator id;

    for( id=sid ; !n && id!=eid; ++id ) {
      dobj *dp = *id;
      switch( dp->ty ) {
      case dobj::ty_varchar:
      case dobj::ty_tinytext:
      case dobj::ty_text:
      case dobj::ty_mediumtext:
      case dobj::ty_longtext:
      case dobj::ty_varbinary:
      case dobj::ty_tinyblob:
      case dobj::ty_blob:
      case dobj::ty_mediumblob:
      case dobj::ty_longblob:
      case dobj::ty_media:
        if( !n++ ) fprintf(sp,"  v_init();\n");
        break;
      default:
        break;
      }
    }

    for( id=sid ; id!=eid; ++id ) {
      dobj *dp = *id;
      const char *dnm = dp->name->c_str();
      if( dp->is_autoincr > 0 && dp->idx ) {
        const char *inm = dp->idx->name->c_str();
        fprintf(sp,"  { %s", typ_dobj(dp));
        fprintf(sp," (%sLoc::*fn)() = &%sLoc::%s;\n", tnm, tnm, dnm);
        fprintf(sp,"    %s", typ_dobj(dp));
        fprintf(sp," v = last(\"%s\",(%s", inm, typ_dobj(dp));
        fprintf(sp," (Db::ObjectLoc::*)())fn);  %s(v+1); }\n", dnm);
      }
      if( dp->has_def > 0 ) {
        switch( dp->has_def ) {
        case dobj::def_integer:
          fprintf(sp,"  %s(%d);\n", dnm, dp->def.i);
          break;
        case dobj::def_double:
          fprintf(sp,"  %s(%f);\n", dnm, dp->def.d);
          break;
        case dobj::def_string:
          switch( dp->ty ) {
          case dobj::ty_enum: {
            list<string*>::iterator snm = dp->eop.begin();
            list<string*>::iterator enm = dp->eop.end();
            list<string*>::iterator nm;
            for( n=0,nm=snm; nm!=enm && *(*nm)!=*dp->def.s; ++nm ) ++n;
            fprintf(sp,"  %s(%d);\n", dnm, n);
            break; }
          case dobj::ty_boolean:
          case dobj::ty_bit:
          case dobj::ty_tinyint:
          case dobj::ty_smallint:
          case dobj::ty_decimal:
          case dobj::ty_mediumint:
          case dobj::ty_integer:
          case dobj::ty_bigint: {
            fprintf(sp,"  %s(%ld);\n", dnm, strtol(dp->def.s->c_str(),0,0));
            break; }
          case dobj::ty_real:
          case dobj::ty_double:
          case dobj::ty_float: {
            fprintf(sp,"  %s(%f);\n", dnm, strtod(dp->def.s->c_str(),0));
            break; }
          case dobj::ty_date:
          case dobj::ty_time:
          case dobj::ty_timestamp:
          case dobj::ty_binary:
          case dobj::ty_char:
          case dobj::ty_datetime:
          case dobj::ty_year:
          case dobj::ty_varchar:
          case dobj::ty_tinytext:
          case dobj::ty_text:
          case dobj::ty_mediumtext:
          case dobj::ty_longtext:
          case dobj::ty_varbinary:
          case dobj::ty_tinyblob:
          case dobj::ty_blob:
          case dobj::ty_mediumblob:
          case dobj::ty_longblob:
          case dobj::ty_media: {
            fprintf(sp,"  %s((%s", dnm, typ_dobj(dp));
            fprintf(sp," *)\"%s\",%d);\n", dp->def.s->c_str(), (int)dp->def.s->size());
            break; }
          default:
            break;
          }
          break;
        default:
          break;
        }
      }
    }
    fprintf(sp,"  return 0;\n");
    fprintf(sp,"}\n");
    fprintf(sp,"\n");
    fprintf(sp,"int %sLoc::Construct()\n", tnm);
    fprintf(sp,"{\n");
    fprintf(sp,"  if_err( insertProhibit() );\n");
    fprintf(sp,"  if_err( construct() );\n");
    n = 0;
    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *knm = ip->name->c_str();
      fprintf(sp,"  { rkey_%s rkey(*this);\n",knm);
      fprintf(sp,"    if_err( entity->index(\"%s\")->Insert(rkey,(void*)_id()) ); }\n", knm);
    }
    fprintf(sp,"  if_err( insertCascade() );\n");
    fprintf(sp,"  return 0;\n");
    fprintf(sp,"}\n");
    fprintf(sp,"\n");
    fprintf(sp,"int %sLoc::Destruct()\n", tnm);
    fprintf(sp,"{\n");
    fprintf(sp,"  if_err( deleteProhibit() );\n");
    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *knm = ip->name->c_str();
      fprintf(sp,"  { rkey_%s rkey(*this);\n",knm);
      fprintf(sp,"    if_err( entity->index(\"%s\")->Delete(rkey) ); }\n",knm);
    }
    fprintf(sp,"  if_err( destruct() );\n");
    fprintf(sp,"  if_err( deleteCascade() );\n");
    fprintf(sp,"  return 0;\n");
    fprintf(sp,"}\n");
    fprintf(sp,"\n");
    fprintf(sp,"void %sLoc::Deallocate()\n", tnm);
    fprintf(sp,"{\n");
    n = 0;
    for( id=sid ; !n && id!=eid; ++id ) {
      dobj *dp = *id;
      switch( dp->ty ) {
      case dobj::ty_varchar:
      case dobj::ty_tinytext:
      case dobj::ty_text:
      case dobj::ty_mediumtext:
      case dobj::ty_longtext:
      case dobj::ty_varbinary:
      case dobj::ty_tinyblob:
      case dobj::ty_blob:
      case dobj::ty_mediumblob:
      case dobj::ty_longblob:
      case dobj::ty_media:
        if( !n++ ) fprintf(sp,"  v_del();\n");
        break;
      default:
        break;
      }
    }
    fprintf(sp,"  deallocate();\n");
    fprintf(sp,"}\n");
    fprintf(sp,"\n");
  }

  fprintf(sp,"\n");
  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"create(const char *dfn)\n");
  fprintf(sp,"{\n");
  fprintf(sp,"  dfd = ::open(dfn,O_RDWR+O_CREAT+O_TRUNC+no_atime,0666);\n");
  fprintf(sp,"  if( dfd < 0 ) { perror(dfn); return -1; }\n");
  fprintf(sp,"  int ret = db_create();\n");
  fprintf(sp,"  close();\n");
  fprintf(sp,"  return ret;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");
  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"db_create()\n");
  fprintf(sp,"{\n");
  fprintf(sp,"  if_ret( Db::make(dfd) );\n");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,"  if_ret( %s.new_entity(\"%s\", sizeof(%sObj)) );\n", tnm, tnm, tnm);
    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *nm = ip->name->c_str();
      if( ip->is_dir > 0 )
        fprintf(sp,"  if_ret( %s.add_dir_index(\"%s\", %sLoc::key_%s::size()) );\n",
          tnm, nm, tnm, nm);
      else
        fprintf(sp,"  if_ret( %s.add_ind_index(\"%s\") );\n", tnm, nm);
    }
    fprintf(sp,"\n");
  }
  fprintf(sp,"  if_ret( Db::commit(1) );\n");
  fprintf(sp,"  return 0;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");

  fprintf(sp,"%s::\n", tdb);
  fprintf(sp,"%s()\n", tdb);
  fprintf(sp," : dfd(-1), dkey(-1), no_atime(getuid()?0:O_NOATIME), objects(0)");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,",\n   %s(this)", tnm);
    fprintf(sp,", %c%s(%s)", tolower(tnm[0]), &tnm[1], tnm);
  }
  fprintf(sp,"\n");
  fprintf(sp,"{");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,"\n  objects = new ObjectList(objects, %c%s);",
      tolower(tnm[0]), &tnm[1]);
  }
  fprintf(sp,"\n");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    list<dobj*>::iterator sid = tp->dop.begin();
    list<dobj*>::iterator eid = tp->dop.end();
    list<dobj*>::iterator id;
    for( id=sid ; id!=eid; ++id ) {
      dobj *dp = *id;
      const char *dnm = dp->name->c_str();
      switch( dp->ty ) {
      case dobj::ty_varchar:
      case dobj::ty_tinytext:
      case dobj::ty_text:
      case dobj::ty_mediumtext:
      case dobj::ty_longtext:
      case dobj::ty_varbinary:
      case dobj::ty_tinyblob:
      case dobj::ty_blob:
      case dobj::ty_mediumblob:
      case dobj::ty_longblob:
      case dobj::ty_media: {
        fprintf(sp,"  %s.add_vref((vRef)&%sObj::v_%s);\n", tnm, tnm, dnm);
        break; }
      default:
        break;
      }
    }
  }
  fprintf(sp,"}\n");
  fprintf(sp,"\n");

  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"open(const char *dfn, int key)\n");
  fprintf(sp,"{\n");
  fprintf(sp,"  dfd = ::open(dfn,O_RDWR+no_atime);\n");
  fprintf(sp,"  if( dfd < 0 ) { perror(dfn); return errNotFound; }\n");
  fprintf(sp,"  if( (dkey=key) >= 0 ) Db::use_shm(1);\n");
  fprintf(sp,"  int ret = Db::open(dfd, dkey);\n");
  fprintf(sp,"  if( !ret ) ret = db_open();\n");
  fprintf(sp,"  if( ret ) close();\n");
  fprintf(sp,"  return ret;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");

  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"db_open()\n");
  fprintf(sp,"{\n");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,"  if_ret( %s.get_entity(\"%s\") );\n", tnm, tnm);
    for( idx=sidx; idx!=eidx; ++idx ) {
      iobj *ip = *idx;
      if( ip->tbl != tp ) continue;
      const char *nm = ip->name->c_str();
      fprintf(sp,"  if_ret( %s.get_index(\"%s\") );\n", tnm, nm);
    }
    fprintf(sp,"\n");
  }
  fprintf(sp,"  if_ret( Db::start_transaction() );\n");
  fprintf(sp,"  return 0;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");
  fprintf(sp,"void %s::\n",tdb);
  fprintf(sp,"close()\n");
  fprintf(sp,"{\n");
  fprintf(sp,"  Db::close();\n");
  fprintf(sp,"  if( dfd >= 0 ) { ::close(dfd); dfd = -1; }\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");
  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"access(const char *dfn, int key, int rw)\n");
  fprintf(sp,"{\n");
  fprintf(sp,"  if( key < 0 ) return Db::errInvalid;\n");
  fprintf(sp,"  dfd = ::open(dfn,O_RDWR+no_atime);\n");
  fprintf(sp,"  if( dfd < 0 ) { perror(dfn); return Db::errNotFound; }\n");
  fprintf(sp,"  dkey = key;  Db::use_shm(1);\n");
  fprintf(sp,"  int ret = Db::attach(dfd, dkey, rw);\n");
  fprintf(sp,"  if( !ret ) ret = db_access();\n");
  fprintf(sp,"  else if( ret == errNotFound ) {\n");
  fprintf(sp,"    ret = Db::open(dfd, dkey);\n");
  fprintf(sp,"    if( !ret ) ret = db_open();\n");
  fprintf(sp,"    if( !ret ) ret = Db::attach(rw);\n");
  fprintf(sp,"  }\n");
  fprintf(sp,"  if( ret ) close();\n");
  fprintf(sp,"  return ret;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");
  fprintf(sp,"int %s::\n",tdb);
  fprintf(sp,"db_access()\n");
  fprintf(sp,"{\n");
  for( it=sit ; it!=eit; ++it ) {
    tobj *tp = *it;
    const char *tnm = tp->name->c_str();
    fprintf(sp,"  if_ret( %s.get_entity(\"%s\") );\n", tnm, tnm);
  }
  fprintf(sp,"  return 0;\n");
  fprintf(sp,"}\n");
  fprintf(sp,"\n");
  fprintf(sp,"\n");
  printf("ended on line %d - ",cp.no);
  for( int i=0, n=cp.i; i<n; ++i ) printf("%c",cp.line[i]);
  printf("|");
  for( int i=cp.i, n=cp.n; i<n; ++i ) printf("%c",cp.line[i]);
  printf("\n");
  return 0;
}

