#define MEKB_RECURSE_IMPL
#include "../src/recurse.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

bool print_dir(const char *path, struct stat st, int depth, int ddepth, void *data) {
	(void) st;
	(void) data;
	if (ddepth < 0) return true;
	for (int i = 0; i < depth; ++i) {
		printf("│ ");
	}
	if (ddepth > 0) {
		printf("├─┐");
	} else {
		printf("├─╴");
	}
	printf("%s\n", path);
	return true;
}

int main() {
	mekb_recurse(".", print_dir, NULL, MEKB_ALLOC(malloc, realloc, free));
}
