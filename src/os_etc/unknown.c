#include <stdint.h>
#include <string.h>
#include "../printer.h"

uint64_t timeMillis() {
	return 0;
}

void sleepMillis(int64_t millis) {
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
