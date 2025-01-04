#include <limits.h>
#include <stdlib.h>
#include <string.h>

static char *absolutePathOverlap(const char *path, char *buff, size_t size) {
	char temp[PATH_MAX] = {0};
	strncpy(temp, path, size);
	return realpath(temp, buff);
}


char *absolutePath(const char *path, char *buff, size_t size) {
	if (size < PATH_MAX) {
		abort();
	}
	if (path >= buff && path < buff + size) {
		return absolutePathOverlap(path, buff, size);
	}
	return realpath(path, buff);
}
