#ifndef MEKB_UTF8_ONCE
#define MEKB_UTF8_ONCE
#include <stddef.h>
#include <stdint.h>

#include "util.h" // depends on util.h

// returns number of bytes read, or negative value on invalid utf-8 sequence
// gets codepoint from utf-8 byte sequence
int mekb_utf8_decode(mekb_read_byte read_byte, void *data, uint32_t *out);

// returns number of bytes written, or negative value on invalid codepoint
// writes utf-8 byte sequence from codepoint
// *out must be at least 4 bytes long
int mekb_utf8_encode(uint32_t codepoint, char *out);

#endif
#ifdef MEKB_UTF8_IMPL
#ifndef MEKB_UTF8_IMPL_ONCE
#define MEKB_UTF8_IMPL_ONCE
int mekb_utf8_decode(mekb_read_byte read_byte, void *data, uint32_t *out) {
	unsigned char buf[4];
	int c;
	uint32_t codepoint = 0;

	// read first byte
	if ((c = read_byte(data)) < 0) return c;
	buf[0] = c;

	if ((buf[0] & 0x80) == 0x00) { // ASCII
		codepoint = buf[0];
		*out = codepoint;
		return 1;
	}

	// read second byte
	if ((c = read_byte(data)) < 0) return c;
	buf[1] = c;

	if ((buf[1] & 0xC0) != 0x80) return -1; // invalid continuation byte

	if ((buf[0] & 0xE0) == 0xC0) { // 2-byte sequence
		codepoint = (buf[0] & 0x1F) << 6;
		codepoint |= buf[1] & 0x3F;
		if (codepoint < 0x80) return -1; // overlong encoding
		*out = codepoint;
		return 2;
	}

	// read third byte
	if ((c = read_byte(data)) < 0) return c;
	buf[2] = c;

	if ((buf[2] & 0xC0) != 0x80) return -1;

	if ((buf[0] & 0xF0) == 0xE0) { // 3-byte sequence
		codepoint = (buf[0] & 0x0F) << 12;
		codepoint |= (buf[1] & 0x3F) << 6;
		codepoint |= buf[2] & 0x3F;
		if (codepoint < 0x800) return -1; // overlong encoding
		*out = codepoint;
		return 3;
	}

	// read fourth byte
	if ((c = read_byte(data)) < 0) return c;
	buf[3] = c;

	if ((buf[3] & 0xC0) != 0x80) return -1;

	if ((buf[0] & 0xF8) == 0xF0) { // 4-byte sequence
		codepoint = (buf[0] & 0x07) << 18;
		codepoint |= (buf[1] & 0x3F) << 12;
		codepoint |= (buf[2] & 0x3F) << 6;
		codepoint |= buf[3] & 0x3F;
		if (codepoint < 0x10000) return -1;  // overlong encoding
		if (codepoint > 0x10FFFF) return -1; // codepoint out of range
		*out = codepoint;
		return 4;
	}

	return -1; // invalid sequence
}

int mekb_utf8_encode(uint32_t codepoint, char *out) {
	if (codepoint <= 0x7f) {
		out[0] = codepoint;
		return 1;
	}
	if (codepoint <= 0x7ff) {
		out[0] = 0xC0 | (codepoint >> 6);
		out[1] = 0x80 | (codepoint & 0x3F);
		return 2;
	}
	if (codepoint <= 0x0ffff) {
		out[0] = 0xE0 | (codepoint >> 12);
		out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[2] = 0x80 | (codepoint & 0x3F);
		return 3;
	}
	if (codepoint <= 0x10FFFF) {
		out[0] = 0xF0 | (codepoint >> 18);
		out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[3] = 0x80 | (codepoint & 0x3F);
		return 4;
	}
	return -1; // codepoint out of range
}
#endif
#endif
