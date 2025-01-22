#define MEKB_UTF8_IMPL
#include "../src/utf8.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "test.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

struct str {
	char *utf8;
	uint32_t codepoint;
	bool fail;
	int pos, len;
};

// read char from string
int read_char(void *data) {
	struct str *str = (struct str *) data;
	if (str->pos >= str->len) return EOF;
	return (unsigned char) str->utf8[str->pos++];
}

int main() {
#define STR(utf8_, codepoint_, fail_) {utf8_, codepoint_, fail_, 0, 0}
	struct str str[] = {
	        STR("\x00", 0x00, false),
	        STR("\x7F", 0x7F, false),
	        STR("\xc3\xbf", 0xFF, false),
	        STR("\xc8\xb6", 0x0236, false),
	        STR("\xea\xaf\x8d", 0xABCD, false),
	        STR("\xf0\x9f\x98\x80", 0x1F600, false),
	        STR("\xf0\x9f\x98\x81", 0x1F601, false),
	        STR("\xf0\x9f\x98\x82", 0x1F602, false),
	        STR("\u1F60", 0x1F60, false),
	        STR("\u0226", 0x0226, false),
	        STR("\uABCD", 0xABCD, false),
	        STR("\u1127", 0x1127, false),
	        STR("\x80", 0, true),
	        STR("\xe2\xa9", 0, true),
	        STR("\xf0\x9f\x98\x00", 0, true),
	        STR("\xf0\xe0", 0, true),
	        STR("\xc0\xae", 0, true),             // overlong
	        STR("\xe0\x80\xae", 0, true),         // overlong
	        STR("\xf0\x80\x80\xae", 0, true),     // overlong
	        STR("\xf4\x90\x80\x80", 0x110000, true), // out of range
	        STR("\xf4\x90\x80", 0, true),            // truncated
	        STR(NULL, 0, false)};
#undef STR

	int ret = 0;
	for (int i = 0; str[i].utf8; i++) {
		if (str[i].fail) {
			printf("Testing decoding of %s - index %i (expected failure)\n", str[i].utf8, i);
		} else {
			printf("Testing decoding of %s - U+%" PRIu32 " - index %i\n", str[i].utf8, str[i].codepoint, i);
		}

		str[i].pos = 0;
		str[i].len = strlen(str[i].utf8);
		if (str[i].len < 1) str[i].len = 1; // \x00

		// test decoding
		uint32_t actual_codepoint;
		if (mekb_utf8_decode(read_char, &str[i], &actual_codepoint) < 0) {
			if (str[i].fail) goto encoding;
			eprintf("Error reading utf8: %i\n", i);
			ret = 1;
			goto encoding;
		}
		if (str[i].fail) {
			eprintf("Expected failure: got %" PRIu32 "\n", actual_codepoint);
			ret = 1;
			goto encoding;
		}

		ASSERT(actual_codepoint, str[i].codepoint, FMT_HEX, ret = 1);

	encoding:
		if (str[i].fail) {
			printf("Testing encoding of U+%" PRIu32 " - index %i (expected failure)\n", str[i].codepoint, i);
		} else {
			printf("Testing encoding of %s - U+%" PRIu32 " - index %i\n", str[i].utf8, str[i].codepoint, i);
		}

		if (str[i].fail && str[i].codepoint == 0) continue;

		// test encoding
		char actual_utf8[4];
		int actual_len = mekb_utf8_encode(str[i].codepoint, actual_utf8);
		if (actual_len < 0) {
			if (str[i].fail) continue;
			eprintf("Error writing utf8: %i\n", i);
			ret = 1;
			continue;
		}
		if (str[i].fail) {
			eprintf("Expected failure: got %i\n", actual_len);
			ret = 1;
			continue;
		}

		if (actual_len != str[i].len) {
			eprintf("Output length mismatch: %i\n", i);
			ret = 1;
			continue;
		}

		for (size_t j = 0; j < (size_t) actual_len; ++j) {
			if (j >= (size_t) str[i].len) {
				eprintf("Output too long\n");
				ret = 1;
				break;
			}
			ASSERT(actual_utf8[j], str[i].utf8[j], FMT_HEX, ret = 1);
		}
	}
	return ret;
}
