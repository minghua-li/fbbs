#ifndef FB_TIME_H
#define FB_TIME_H

#include <inttypes.h>
#include <stdint.h>
#include <time.h>

#define PRIdFBT PRId64

enum DATE_FORMAT {
	DATE_ZH = 0,      ///< "2001��02��03��04:05:06 ������"
	DATE_EN = 1,      ///< "02/03/01 04:05:06"
	DATE_SHORT = 2,   ///< "02.03 04:05"
	DATE_ENWEEK = 4,  ///< "02/03/01 04:05:06 Sat"
	DATE_XML = 8,     ///< "2001-02-03T04:05:06"
	DATE_RSS = 16,    ///< "Sat,03 Feb 2001 04:05:06 +0800"
};

typedef int64_t fb_time_t;

extern char *getdatestring(time_t time, enum DATE_FORMAT mode);

#endif // FB_TIME_H