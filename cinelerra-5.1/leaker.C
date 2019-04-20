#include <stdio.h>
#include <strings.h>

#include <map>
using namespace std;

#if 0
#define STRC(v) printf("==new %p from %p sz %jd\n", v, __builtin_return_address(0), n)
#define STRD(v) printf("==del %p from %p\n", v, __builtin_return_address(0))
void *operator new(size_t n) { void *vp = malloc(n); STRC(vp); bzero(vp,n); return vp; }
void operator delete(void *t) { STRD(t); free(t); }
void operator delete(void *t,size_t n) { STRD(t); free(t); }
void *operator new[](size_t n) { void *vp = malloc(n); STRC(vp); bzero(vp,n); return vp; }
void operator delete[](void *t) { STRD(t); free(t); }
void operator delete[](void *t,size_t n) { STRD(t); free(t); }
#endif


class blob
{
public:
	int64_t adr, from, sz;
	blob(int64_t adr, int64_t from, int64_t sz) {
		this->adr = adr; this->from = from; this->sz = sz;
	}
};
typedef map<int64_t, blob*> recd_map;
typedef recd_map::value_type recd_val;
typedef recd_map::iterator recd_it;

int main(int ac, char **av)
{
	int64_t adr, from, sz;
	recd_map recds;
	char line[65536];
	FILE *fp = stdin;

	while( fgets(line,sizeof(line),fp) ) {
		if( line[0] != '=' ) continue;
		if( line[1] != '=' ) continue;
		if( sscanf(line, "==new %jx from %jx sz %jd\n", &adr, &from, &sz) == 3 ) {
			recds.insert(recd_val(adr, new blob(adr,from,sz)));
			continue;
		}
		if( sscanf(line, "==del %jx from %jx\n", &adr, &from) == 2 ) {
			recd_it ri = recds.lower_bound(adr);
			if( ri == recds.end() || ri->first != adr ) {
				printf("del miss adr %jx\n", adr);
				continue;
			}
			recds.erase(ri);
		}
	}

	int64_t n = recds.size();  sz = 0;
	for( recd_it ri = recds.begin(); ri != recds.end(); ++ri ) sz += ri->second->sz;
	printf("in use: %jd sz %jd\n", n, sz);

	recd_map leaks;
	for( recd_it ri = recds.begin(); ri != recds.end(); ++ri ) {
		adr = ri->second->adr;  from = ri->second->from;  sz = ri->second->sz;
		recd_it li = leaks.lower_bound(from);
		if( li == leaks.end() || li->first != from ) {
			leaks.insert(recd_val(from, new blob(adr,from,sz)));
		}
		else {
			li->second->sz += sz;
		}
	}
	sz = 0;  n = 0;
	for( recd_it li = leaks.begin(); li != leaks.end(); ++li,++n ) {
		printf("==leak at %jx sz %jd\n", li->second->from, li->second->sz);
		sz += li->second->sz;
	}
	printf("leakers: %jd/%jd sz %jd\n", leaks.size(), n, sz);
	return 0;
}

