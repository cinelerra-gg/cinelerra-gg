#ifndef __CSTRDUP_H__
#define __CSTRDUP_H__

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <wctype.h>

static inline char *cstrcat(int n, ...) {
  int len = 0;  va_list va;  va_start(va,n);
  for(int i=0; i<n; ++i) len += strlen(va_arg(va,char*));
  va_end(va);  char *cp = new char[len+1], *bp = cp;  va_start(va,n);
  for(int i=0; i<n; ++i) for(char*ap=va_arg(va,char*); *ap; *bp++=*ap++);
  va_end(va);  *bp = 0;
  return cp;
}
static inline char *cstrdup(const char *cp) {
  return strcpy(new char[strlen(cp)+1],cp);
}

#ifndef lengthof
#define lengthof(ary) ((int)(sizeof(ary)/sizeof(ary[0])))
#endif

static inline int butf8(const char *&cp)
{
	const unsigned char *bp = (const unsigned char *)cp;
	int ret = *bp++;
	if( ret >= 0x80 ) {
		int v = ret - 0xc0;
		static const int64_t sz = 0x5433222211111111;
		int n = v < 0 ? 0 : (sz >> (v&0x3c)) & 0x0f;
		for( int i=n; --i>=0; ret+=*bp++ ) ret <<= 6;
		static const uint32_t ofs[6] = {
			0x00000000U, 0x00003080U, 0x000E2080U,
			0x03C82080U, 0xFA082080U, 0x82082080U
		};
		ret -= ofs[n];
	}
	cp = (const char *)bp;
	return ret;
}
static inline int butf8(unsigned int v, char *&cp)
{
	unsigned char *bp = (unsigned char *)cp;
	if( v >= 0x00000080 ) {
		int i = v < 0x00000800 ? 2 : v < 0x00010000 ? 3 :
			v < 0x00200000 ? 4 : v < 0x04000000 ? 5 : 6;
		int m = 0xff00 >> i;
		*bp++ = (v>>(6*--i)) | m;
		while( --i >= 0 ) *bp++ = ((v>>(6*i)) & 0x3f) | 0x80;
	}
	else
		*bp++ = v;
	int ret = bp - (unsigned char *)cp;
	cp = (char *)bp;
	return ret;
}

static inline int bstrcasecmp(const char *ap, const char *bp)
{
	int a, b, ret;
	do {
		a = towlower(butf8(ap));  b = towlower(butf8(bp));
	} while( !(ret=a-b) && a && b );
	return ret;
}

static inline const char *bstrcasestr(const char *src, const char *tgt)
{
	int ssz = strlen(src), tsz = strlen(tgt), ret = 0;
	const char *cp = tgt;
	wchar_t wtgt[tsz + 1], *tp = wtgt;
	while( *cp ) *tp++ = towlower(butf8(cp));
	for( tsz=tp-wtgt; ssz>=tsz; ++src,--ssz ) {
		cp = src;   tp = wtgt;
		for( int i=tsz; --i>=0 && !(ret=towlower(butf8(cp))-*tp); ++tp );
		if( !ret ) return src;
	}
	return 0;
}

#endif
