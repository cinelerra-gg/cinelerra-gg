#ifndef _MEDIA_DB_INC_
#define _MEDIA_DB_INC_

class Dele;
class Deletions;
class Clip;
class Clips;
class Snip;
class Snips;
class MediaDb;

class theDb;
#define MEDIA_DB "/cinelerra/media.db"
#define MEDIA_SHM_KEY 34543

// correlation margin for clip position jiggling
#define TRANSITION_MARGIN 1.0
// merge cuts when clip is within clip margin seconds of last clip
#define CLIP_MARGIN 2.0

// pixel weight match err limit
#define MEDIA_WEIGHT_ERRLMT 3.
// search mean limit
#define MEDIA_MEAN_ERRLMT 4.
// search standard deviation err limit
#define MEDIA_STDDEV_ERRLMT 2.
// search centroid err limit
#define MEDIA_XCENTER_ERRLMT 1.
#define MEDIA_YCENTER_ERRLMT 1.
// search distance err limit
#define MEDIA_SEARCH_ERRLMT 6.
// db dupl chk search radius
#define MEDIA_SEARCH_DIST 256

// db frame chk distance err limit
#define MEDIA_FRAME_ERRLMT 0.25
// db frame chk search radius
#define MEDIA_FRAME_DIST 256

// db clip frame sample standard size
#define SWIDTH 80
#define SHEIGHT 45
#define SFRM_SZ (SWIDTH*SHEIGHT)

#endif
