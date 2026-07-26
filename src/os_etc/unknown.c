#include <stdint.h>
#include <string.h>
#include "../printer.h"

uint64_t timeMillis() {
	return 0;
}

void sleepMillis(int64_t millis) {
	(void) millis;
}

int localZoneName(char *dst, size_t dstSize) {
	if (dst != NULL && dstSize > 0) {
		dst[0] = 0;
	}
	return 0;
}

int64_t localZoneOffset(int64_t timeMs, int *daylight) {
	(void) timeMs;
	if (daylight != NULL) {
		*daylight = 0;
	}
	return 0;
}


// use a fake path to make the tests work.
char *absolutePath(const char *path, char *buff, size_t size) {
	strncpy(buff, path, size - 2);
	return buff;
}


int importLib(rtContext ctx, const char *path) {
	error(ctx, NULL, 0, "Error opening library: %s", "Plugins are not supported");
	(void) path;
	return -1;
}

void closeLibs(rtContext ctx) {
	(void) ctx;
}
