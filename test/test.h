#ifndef TEST_H
#define TEST_H
#include <stdio.h>
#include <inttypes.h>

// number format specifiers
// cast to size_t to avoid format warnings
#define NUM(x) ((size_t) (x))

#define ASSERT(actual, expected, format, failure)                               \
	{                                                                           \
		int actual_ = (actual);                                                 \
		int expected_ = (expected);                                             \
		if ((actual_) == (expected_)) {                                         \
			printf("Assertion passed: ");                                       \
			printf(format " == " format " ... ", NUM(actual_), NUM(expected_)); \
			printf("%s == %s\n", #actual, #expected);                           \
		} else {                                                                \
			printf("Assertion failed: ");                                       \
			printf(format " == " format " ... ", NUM(actual_), NUM(expected_)); \
			printf("%s == %s\n", #actual, #expected);                           \
			failure;                                                            \
		}                                                                       \
	}

// custom format specifiers
#define FMT_PRE "%"
#define FMT_CHAR "#"
#define FMT_DEC FMT_PRE PRIdPTR
#define FMT_INT FMT_PRE PRIiPTR
#define FMT_UINT FMT_PRE PRIuPTR
#define FMT_HEX FMT_PRE FMT_CHAR PRIxPTR
#define FMT_HEX_UPPER FMT_PRE FMT_CHAR PRIXPTR
#define FMT_OCTAL FMT_PRE FMT_CHAR PRIoPTR

#define ASSERT_TRUE(actual, failure) ASSERT(actual, ==, true, FMT_INT, failure)
#define ASSERT_FALSE(actual, failure) ASSERT(actual, ==, false, FMT_INT, failure)
#endif // TEST_H
