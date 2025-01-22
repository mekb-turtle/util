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

	struct mekb_alloc alloc = MEKB_ALLOC(malloc, realloc, free);
	struct mekb_ini_list *list = NULL;
	r = mekb_ini_parse((mekb_read_byte) read_byte, (void *) &data_, &list, alloc);
	if (r < 0) {
		eprintf("Error parsing ini\n");
		return r;
	} else
		r = 0;

	char *out = NULL;

	r = mekb_ini_serialize(list, &out, alloc);
	if (r < 0) {
		eprintf("Error serializing ini\n");
		return r;
	}

	size_t out_len = strlen(out);
	if (out_len != (size_t) r) {
		ASSERT(out_len, r, FMT_HEX, r = 1);
		eprintf("Output length mismatch\n");
		r = 1;
	} else
		r = 0;

	for (size_t i = 0; i < out_len; ++i) {
		if (i > strlen(expected)) {
			eprintf("Output too long\n");
			r = 1;
			break;
		}
		ASSERT(out[i], expected[i], FMT_HEX, r = 1);
	}

	printf("Output:\n%s", out);

	free(out);
	mekb_free_ini(list, alloc.free);

	return r;
}
