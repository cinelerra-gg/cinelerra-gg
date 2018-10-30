#ifndef INDEXSTATE_INC
#define INDEXSTATE_INC

// magic number "mkr"  version = "1"
#define MARKER_MAGIC_VERSION "mkr1"

class IndexItem;
class IndexMark;
class IndexChannel;
class IndexChannels;
class IndexEntry;
class IndexEntries;
class IndexMarks;
class IndexMarkers;
class IndexState;

// Index statuses
#define INDEX_READY     0
#define INDEX_NOTTESTED 1
#define INDEX_BUILDING  2
#define INDEX_TOOSMALL  3

#define MARKERS_NOTTESTED 0
#define MARKERS_READY   1

#endif
