#define MEKB_INI_PARSE_IMPL
#include "../src/ini_parse.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

static int read_byte(void *data) {
	// read next data from string
	const char **d = (const char **) data;

	char c = **d;
	if (!c) return EOF;
	++(*d);   // move to next char
	return c; // return byte
}

int main() {
	int r = 0;

	const char *data = "\n\
\n\
# comment\n\
foo=bar2 ; comment again\n\
bar=foo\n\
[section1]  \n\
foo=bar\n\
; comment\n\
never=  gonna\n\
  [ section2  ]  \n\
[  section3  ]\n\
give   = you\n\
up\n\
\n\
		";

	const char *data_ = data;

	const char *expected = "\
foo=bar2\n\
bar=foo\n\
[section1]\n\
foo=bar\n\
never=gonna\n\
[section2]\n\
[section3]\n\
give=you\n\
up\n";

	struct mekb_alloc alloc = {.malloc = malloc, .realloc = realloc, .free = free};
	struct mekb_ini_list *list = NULL;
	r = mekb_ini_parse((mekb_read_byte) read_byte, (void *) &data_, &list, alloc);
	if (r < 0) {
		eprintf("Error parsing ini\n");
		return r;
	} else
		r = 0;

#define OUT_SIZE (1024)
	char *out = malloc(OUT_SIZE); // should be enough for this test
	if (out == NULL) {
		mekb_free_ini(list, alloc.free);
		eprintf("malloc\n");
		return 1;
	}

	out[0] = '\0';

	// append to out string
#define catprintf(...)                                            \
	{                                                             \
		size_t len = strlen(out);                                 \
		int r = snprintf(out + len, OUT_SIZE - len, __VA_ARGS__); \
		if (r < 0 || r >= OUT_SIZE - (int) len) {                 \
			mekb_free_ini(list, alloc.free);                      \
			free(out);                                            \
			eprintf("snprintf error\n");                          \
			return 1;                                             \
		}                                                         \
	}

	for (struct mekb_ini_list *l = list; l; l = l->next) {
		if (l->header) {
			catprintf("[%s]\n", l->header);
		}
		for (struct mekb_keyval_list *keyval = l->entries; keyval; keyval = keyval->next) {
			char *key = keyval->key;
			char *val = keyval->value;
			if (val) {
				catprintf("%s=%s\n", key, val);
			} else {
				catprintf("%s\n", key);
			}
		}
	}

	mekb_free_ini(list, alloc.free);

	for (size_t i = 0; i < strlen(out); ++i) {
		if (i > strlen(expected)) {
			eprintf("Output too long\n");
			r = 1;
			break;
		}
		ASSERT(out[i], expected[i], FMT_HEX, r = 1);
	}

	printf("Output:\n%s\n", out);

	free(out);

	return r;
}
