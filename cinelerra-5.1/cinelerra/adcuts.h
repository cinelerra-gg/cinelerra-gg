#ifndef _ADCUTS_H_
#define _ADCUTS_H_

#include "adcuts.inc"
#include "bcwindowbase.inc"
#include "filexml.inc"
#include "file.inc"
#include "linklist.h"

class AdCut : public ListItem<AdCut>
{
public:
        double time;
	int action;

	AdCut(double t, int a) : time(t), action(a) {}
	~AdCut() {}
};

class AdCuts : public List<AdCut>
{
	int save(FileXML &xml);
	int load(FileXML &xml);
public:
	int fd, pid;
	char filename[BCTEXTLEN];

	void write_cuts(const char *filename);
	static AdCuts *read_cuts(const char *filename);

        AdCuts(int pid, int fd, const char *fn);
        ~AdCuts();
};

#endif
