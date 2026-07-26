#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

uint64_t timeMillis() {
	uint64_t now;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	now = tv.tv_sec * (uint64_t)1000;
	now += tv.tv_usec / (uint64_t)1000;
	return now;
}

void sleepMillis(int64_t millis) {
	struct timespec ts;
	ts.tv_sec = millis / 1000;
	ts.tv_nsec = (millis % 1000) * 1000000;
	nanosleep(&ts, NULL);
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

// Local broken-down time + offset seconds east of UTC for UTC epoch milliseconds.
// Returns non-zero on success. offsetSec may be NULL when only tm is needed.
static int localTimeParts(int64_t timeMs, struct tm *localTm, int64_t *offsetSec) {
	time_t secs = (time_t) (timeMs / 1000);
	memset(localTm, 0, sizeof(*localTm));

	if (localtime_r(&secs, localTm) == NULL) {
		if (offsetSec != NULL) {
			*offsetSec = 0;
		}
		return 0;
	}

	if (offsetSec == NULL) {
		return 1;
	}

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__linux__)
	*offsetSec = (int64_t) localTm->tm_gmtoff;
#else
	// Fallback using libc timezone globals after tzset().
	tzset();
	*offsetSec = -((int64_t) timezone);
	if (localTm->tm_isdst > 0) {
		*offsetSec += 3600;
	}
#endif
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

	const char *src = NULL;
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__linux__)
	src = localTm.tm_zone;
#endif
	if (src == NULL || src[0] == 0) {
		formatUtcOffsetName(dst, dstSize, offsetSec);
	} else {
		snprintf(dst, dstSize, "%s", src);
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
