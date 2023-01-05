/**
 * Utility macros and functions.
 */

#ifndef CMPL_UTILS_H
#define CMPL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmpl.h"

/// Dynamic array
struct arrBuffer {
	char *ptr;         // data
	size_t esz;        // element size
	size_t cap;        // capacity
	size_t cnt;        // allocated size
};

int initBuff(struct arrBuffer *buff, size_t initCount, size_t elemSize);
static inline void *getBuff(struct arrBuffer *buff, size_t idx) {
	size_t pos = idx * buff->esz;
	if (pos >= buff->cap || buff->ptr == NULL) {
		return NULL;
	}
	return buff->ptr + pos;
}
void *insBuff(struct arrBuffer *buff, size_t idx, void *data);
void *setBuff(struct arrBuffer *buff, size_t idx, void *data);
void freeBuff(struct arrBuffer *buff);


/// Get the length of an array, the number of elements it contains
#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))

/// Get the offset of a field of a struct
#define offsetOf(__RECORD, __FIELD) ((size_t) &((__RECORD*)NULL)->__FIELD)

/// Get the size of a field of a struct
#define sizeOf(__RECORD, __FIELD) sizeof(((__RECORD*)NULL)->__FIELD)


/// Get the current UTC time in milliseconds
uint64_t timeMillis();

/// Go to sleep for a while
void sleepMillis(int64_t millis);

#ifdef __cplusplus
}
#endif
#endif
