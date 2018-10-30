#ifndef __PLUGINFCLIENT_INC__
#define __PLUGINFCLIENT_INC__

struct AVFilter;
typedef struct AVFilter AVFilter;
struct AVFilterGraph;
typedef struct AVFilterGraph AVFilterGraph;

class PluginFClient_Opt;
class PluginFFilter;
class PluginFClientConfig;
class PluginFClient_OptName;
class PluginFClient_OptValue;
class PluginFClient_OptPanel;
class PluginFClientUnits;
class PluginFClientText;
class PluginFClientApply;
class PluginFClientWindow;
class PluginFClient;
class PluginFAClient;
class PluginFVClient;

#ifndef  FLT_MAX
#define FLT_MAX 3.40282346638528859812e+38F
#endif

#ifndef  FLT_MIN
#define FLT_MIN 1.17549435082228750797e-38F
#endif

#ifndef  DBL_MAX
#define DBL_MAX ((double)1.79769313486231570815e+308L)
#endif

#ifndef  DBL_MIN
#define DBL_MIN ((double)2.22507385850720138309e-308L)
#endif

#endif
