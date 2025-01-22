#ifndef MEKB_UTIL_ONCE
#define MEKB_UTIL_ONCE
// structure to store memory allocation functions
struct mekb_alloc {
	void *(*malloc)(size_t);
	void *(*realloc)(void *, size_t);
	void (*free)(void *);
};

#define MEKB_ALLOC(malloc_, realloc_, free_) ((struct mekb_alloc) {.malloc = malloc_, .realloc = realloc_, .free = free_})

// should return a byte value, EOF on end of file, or a negative value on error
typedef int (*mekb_read_byte)(void *data);
#endif
