
#include "adcuts.h"
#include "filexml.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

AdCuts::
AdCuts(int pid, int fd, const char *fn)
{
	this->pid = pid;
	this->fd = fd;
	memset(this->filename,0,sizeof(this->filename));
	if( fn ) strcpy(this->filename,fn);
}

AdCuts::
~AdCuts()
{
	if( fd < 0 ) return;
	::close(fd);
	if( first ) {
		char cut_filename[BCTEXTLEN];
		strcpy(cut_filename, filename);
		strcpy(strrchr(cut_filename, '.'),".cut");
		write_cuts(cut_filename);
		printf(_("cuts to %s complete\n"),cut_filename);
	}
	else
		::remove(filename);
}


int AdCuts::
load(FileXML &xml)
{
        for(;;) {
                if( xml.read_tag() != 0 ) return 1;
                if( !xml.tag.title_is("CUT") ) break;
                double time = xml.tag.get_property("TIME", (double)0.0);
                int action = xml.tag.get_property("ACTION", (int)0);
                append(new AdCut(time, action));
        }
        return 0;
}

AdCuts *AdCuts::
read_cuts(const char *filename)
{
        FileXML xml;
        if( xml.read_from_file(filename, 1) ) return 0;
        do {
                if( xml.read_tag() ) return 0;
        } while( !xml.tag.title_is("CUTS") );

	int pid = xml.tag.get_property("PID", (int)-1);
	const char *file = xml.tag.get_property("FILE");
	AdCuts *cuts = new AdCuts(pid, -1, file);
	if( cuts->load(xml) || !xml.tag.title_is("/CUTS") ) {
		 delete cuts; cuts = 0;
	}
	return cuts;
}

int AdCuts::
save(FileXML &xml)
{
	xml.tag.set_title("CUTS");
	xml.tag.set_property("PID", pid);
	xml.tag.set_property("FILE", filename);
	xml.append_tag();
	xml.append_newline();

	for( AdCut *cut=first; cut; cut=cut->next ) {
		xml.tag.set_title("CUT");
		xml.tag.set_property("TIME", cut->time);
		xml.tag.set_property("ACTION", cut->action);
		xml.append_tag();
		xml.tag.set_title("/CUT");
		xml.append_tag();
		xml.append_newline();
	}

	xml.tag.set_title("/CUTS");
	xml.append_tag();
	xml.append_newline();
	return 0;
}

void AdCuts::
write_cuts(const char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if( fp != 0 ) {
		FileXML xml;
		save(xml);
		xml.terminate_string();
		xml.write_to_file(fp);
		fclose(fp);
	}
}

