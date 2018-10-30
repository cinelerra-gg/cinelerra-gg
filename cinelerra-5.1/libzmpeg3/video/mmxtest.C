#include "../libzmpeg3.h"

int mpeg3_mmx_test()
{
  int result = 0;
#ifdef HAVE_MMX
  FILE *proc;
  char string[zmpeg3_t::STRLEN];

  if(!(proc = fopen(zmpeg3_t::PROC_CPUINFO, "r"))) {
    perrs("%s",zmpeg3_t::PROC_CPUINFO);
    return 0;
  }
  
  while( !feof(proc) && fgets(string, sizeof(string), proc) ) {
    /* Get the flags line */
    if(!strncasecmp(string, "flags", 5)) {
      result = !strstr(string, "mmx") ? 0 : 1;
      break;
    }
  }
  fclose(proc);
#endif
  return result;
}

