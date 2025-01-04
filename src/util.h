/**
 * Utility macros and functions.
 */
#ifndef CMPL_UTILS_H
#define CMPL_UTILS_H

#include <stdint.h>
#include <stddef.h>

/// Get the size of a field from a struct
#define sizeOf(__RECORD, __FIELD) sizeof(((__RECORD*)NULL)->__FIELD)

/// Get the offset of a field from a struct
#define offsetOf(__RECORD, __FIELD) ((size_t) &((__RECORD*)NULL)->__FIELD)

/// Get the length of an array, the number of elements it contains
#define lengthOf(__ARRAY) (sizeof(__ARRAY) / sizeof(*(__ARRAY)))


/// Utility function to align an offset
static inline size_t padOffset(size_t offs, size_t align) {
	return (offs + (align - 1)) & -align;
}

/// Utility function to align a pointer
static inline void *padPointer(void *offs, size_t align) {
	return (void *) padOffset((size_t) offs, align);
}

static inline int32_t bitPop32(uint32_t value) {
	value -= (value >> 1) & 0x55555555;
	value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
	value = (value + (value >> 4)) & 0x0f0f0f0f;
	value += value >> 8;
	value += value >> 16;
	return value & 0x3f;
}

static inline int32_t bitLen32(uint32_t value) {
	int32_t result = 0;
	if ((value >> 16) != 0) {
		result += 16;
		value >>= 16;
	}
	if ((value >> 8) != 0) {
		result += 8;
		value >>= 8;
	}
	if ((value >> 4) != 0) {
		result += 4;
		value >>= 4;
	}
	if ((value >> 2) != 0) {
		result += 2;
		value >>= 2;
	}
	if ((value >> 1) != 0) {
		result += 1;
		value >>= 1;
	}
	return result + value;
}

static inline int32_t bitPop64(uint64_t value) {
	value -= (value >> 1) & 0x5555555555555555;
	value = (value & 0x3333333333333333) + ((value >> 2) & 0x3333333333333333);
	value = (value + (value >> 4)) & 0x0f0f0f0f0f0f0f0f;
	value += value >> 8;
	value += value >> 16;
	value += value >> 32;
	return value & 0x3f;
}

static inline int32_t bitLen64(uint64_t value) {
	int32_t result = 0;
	if ((value >> 32) != 0) {
		result += 32;
		value >>= 32;
	}
	if ((value >> 16) != 0) {
		result += 16;
		value >>= 16;
	}
	if ((value >> 8) != 0) {
		result += 8;
		value >>= 8;
	}
	if ((value >> 4) != 0) {
		result += 4;
		value >>= 4;
	}
	if ((value >> 2) != 0) {
		result += 2;
		value >>= 2;
	}
	if ((value >> 1) != 0) {
		result += 1;
		value >>= 1;
	}
	return result + value;
}

/// Bit scan reverse: position of the Most Significant Bit
static inline int32_t bitsr(uint32_t value) {
	return bitLen32(value) - 1;
}

/// Bit scan forward: position of the Least Significant Bit
static inline int32_t bitsf(uint32_t value) {
	return bitLen32(value & -value) - 1;
}

/** Calculate hash code of a string.
 * @param data data to be hashed.
 * @param size length of data.
 */
unsigned rehash(const char *data, size_t size);

/// Utility function to swap memory
void memSwap(void *dst, void *src, size_t size);

/// Singly linked list
typedef struct list {
	struct list *next;
	const char *data;
} *list;

/// Dynamic array
struct arrBuffer {
	char *ptr;         // data
	size_t esz;        // element size
	size_t cap;        // capacity
	size_t cnt;        // allocated size
};

void initBuff(struct arrBuffer *buff, size_t initCount, size_t elemSize);
static inline void *getBuff(const struct arrBuffer *buff, size_t idx) {
	size_t pos = idx * buff->esz;
	if (pos >= buff->cap || buff->ptr == NULL) {
		return NULL;
	}
	return buff->ptr + pos;
}
void *insBuff(struct arrBuffer *buff, size_t idx, void *data);
void *setBuff(struct arrBuffer *buff, size_t idx, const void *data);
void freeBuff(struct arrBuffer *buff);


// utility functions
char *absolutePath(const char *path, char *buff, size_t size);

char *relativeToCWD(const char *path);

/// Get the current UTC time in milliseconds
uint64_t timeMillis();

/// Go to sleep for a while
void sleepMillis(int64_t millis);

#endif
