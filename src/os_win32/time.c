#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <Windows.h>

uint64_t timeMillis() {
	static const int64_t kTimeEpoc = 116444736000000000LL;
	static const int64_t kTimeScale = 10000;  // 100 ns to us.

	// Although win32 uses 64-bit integers for representing timestamps,
	// these are packed into a FILETIME structure. The FILETIME
	// structure is just a struct representing a 64-bit integer. The
	// TimeStamp union allows access to both a FILETIME and an integer
	// representation of the timestamp. The Windows timestamp is in
	// 100-nanosecond intervals since January 1, 1601.
	typedef union {
		FILETIME ftime;
		int64_t time;
	} TimeStamp;
	TimeStamp time;
	GetSystemTimeAsFileTime(&time.ftime);
	return (time.time - kTimeEpoc) / kTimeScale;
}

void sleepMillis(int64_t milliseconds) {
	Sleep(milliseconds);
}

static void formatUtcOffsetName(char *dst, size_t dstSize, int64_t offsetSec) {
	int64_t total = offsetSec;
	char sign = '+';
	if (total < 0) {
		sign = '-';
		total = -total;
	}
	int hh = (int) (total / 3600);
	int mm = (int) ((total % 3600) / 60);
	if (total == 0) {
		snprintf(dst, dstSize, "UTC");
	} else if (mm == 0) {
		snprintf(dst, dstSize, "UTC%c%02d", sign, hh);
	} else {
		snprintf(dst, dstSize, "UTC%c%02d:%02d", sign, hh, mm);
	}
}

// Bias is minutes west of UTC; daylight bias applies when DST is active.
static int64_t winOffsetSec(const struct tm *localTm) {
	TIME_ZONE_INFORMATION tzi;
	DWORD tzId = GetTimeZoneInformation(&tzi);
	long bias = tzi.Bias;
	if (localTm->tm_isdst > 0 && tzId != TIME_ZONE_ID_UNKNOWN) {
		bias += tzi.DaylightBias;
	} else {
		bias += tzi.StandardBias;
	}
	return -((int64_t) bias) * 60;
}

// Local broken-down time + offset seconds east of UTC for UTC epoch milliseconds.
// Returns non-zero on success. offsetSec may be NULL when only tm is needed.
static int localTimeParts(int64_t timeMs, struct tm *localTm, int64_t *offsetSec) {
	time_t secs = (time_t) (timeMs / 1000);
	memset(localTm, 0, sizeof(*localTm));

	if (localtime_s(localTm, &secs) != 0) {
		if (offsetSec != NULL) {
			*offsetSec = 0;
		}
		return 0;
	}

	if (offsetSec != NULL) {
		*offsetSec = winOffsetSec(localTm);
	}
	return 1;
}

int localZoneName(char *dst, size_t dstSize) {
	if (dst == NULL || dstSize == 0) {
		return 0;
	}

	struct tm localTm;
	int64_t offsetSec = 0;
	if (!localTimeParts((int64_t) timeMillis(), &localTm, &offsetSec)) {
		dst[0] = 0;
		return 0;
	}

	TIME_ZONE_INFORMATION tzi;
	DWORD tzId = GetTimeZoneInformation(&tzi);
	const WCHAR *wname = (localTm.tm_isdst > 0 && tzId == TIME_ZONE_ID_DAYLIGHT)
		? tzi.DaylightName : tzi.StandardName;
	char tmp[64];
	size_t n = wcstombs(tmp, wname, sizeof(tmp) - 1);
	if (n == (size_t) -1) {
		tmp[0] = 0;
	} else {
		tmp[n] = 0;
	}

	if (tmp[0] == 0) {
		formatUtcOffsetName(dst, dstSize, offsetSec);
	} else {
		snprintf(dst, dstSize, "%s", tmp);
	}
	dst[dstSize - 1] = 0;
	return 1;
}

int64_t localZoneOffset(int64_t timeMs, int *daylight) {
	struct tm localTm;
	int64_t offsetSec = 0;
	if (!localTimeParts(timeMs, &localTm, &offsetSec)) {
		if (daylight != NULL) {
			*daylight = 0;
		}
		return 0;
	}

	if (daylight != NULL) {
		*daylight = localTm.tm_isdst > 0;
	}
	return offsetSec * 1000;
}
