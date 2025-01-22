#ifndef MEKB_INI_PARSE_ONCE
#define MEKB_INI_PARSE_ONCE
#include <stddef.h>

#include "util.h" // depends on util.h

struct mekb_keyval_list {
	// value can be NULL if no '=' is present
	char *key, *value;
	struct mekb_keyval_list *next;
};

// linked list structure to store ini file
struct mekb_ini_list {
	char *header; // section header, first item is always NULL for keyvals without a header
	struct mekb_keyval_list *entries;
	struct mekb_ini_list *next; // next section
};

// returns length of line, or a negative value on error
// *out is null terminated
// *eol is set to the character that terminated the line, which is one of the following:
// EOF, '\n', '\r', or '\0' denote end of line
int mekb_read_line(mekb_read_byte read_byte, void *data, char **out, int *eol, struct mekb_alloc alloc);

// returns line with leading and trailing whitespace removed, NULL if line is empty
// uses isspace to determine whitespace
// output string is null terminated
// input string is modified in place, and is terminated by the first instance of term or a null byte
char *mekb_trim_whitespace(char *line, char term);

// returns number of sections parsed, or a negative value on error
// read_byte must return EOF or a negative value eventually, otherwise risk infinite loop
// free with mekb_free_ini
int mekb_ini_parse(mekb_read_byte read_byte, void *data, struct mekb_ini_list **out, struct mekb_alloc alloc);

// frees a single ini entry
void mekb_free_ini_entry(struct mekb_ini_list *list, void (*free)(void *));

// frees the entire ini list
void mekb_free_ini(struct mekb_ini_list *list, void (*free)(void *));

// serialize ini list into a string
// returns length of string written to *out, or a negative value on error
// returns 0 and *out is NULL if list is empty
int mekb_ini_serialize(struct mekb_ini_list *in, char **out, struct mekb_alloc alloc);

#endif
#ifdef MEKB_INI_PARSE_IMPL
#ifndef MEKB_INI_PARSE_IMPL_ONCE
#define MEKB_INI_PARSE_IMPL_ONCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

int mekb_read_line(int (*read_byte)(void *data), void *data, char **out, int *eol, struct mekb_alloc alloc) {
	char *line = NULL;
	size_t i;
	size_t line_len = 256; // initial line capacity
	for (i = 0;; ++i) {
		if (!line || i >= line_len) {
			if (!line) {
				line = alloc.malloc(line_len);
				if (line == NULL) return -1;
			} else {
				// double line capacity
				line_len *= 2;
				char *new_line = alloc.realloc(line, line_len);
				if (new_line == NULL) {
					// realloc fail does not free old memory
					alloc.free(line);
					return -1;
				}
				line = new_line;
			}
		}
		int c = read_byte(data);
		if (c == EOF || c == '\n' || c == '\r' || c == '\0') {
			// end of file/line
			line[i] = '\0';
			if (eol) *eol = c;
			break;
		}
		if (c < 0) {
			// error reading byte
			alloc.free(line);
			return -1;
		}
		line[i] = c;
	}
	if (out) *out = line;
	if (i > INT_MAX) return INT_MAX;
	return i;
}

char *mekb_trim_whitespace(char *line, char term) {
	char *end = strchrnul(line, term); // get end of line
	if (end == line) return NULL;      // empty line
	--end;

	// trim trailing whitespace
	while (isspace(*end) && end > line) --end;
	// trim leading whitespace
	while (isspace(*line) && end > line) ++line;

	if (line == end && isspace(*line)) return NULL; // empty line

	// null terminate
	end[1] = '\0';

	return line;
}

int mekb_ini_parse(mekb_read_byte read_byte, void *data, struct mekb_ini_list **out, struct mekb_alloc alloc) {
	char *line_ = NULL;

	struct mekb_keyval_list *last_keyval = NULL;

	// create new list
	struct mekb_ini_list *list = alloc.malloc(sizeof(struct mekb_ini_list));
	if (list == NULL) return -1;

	list->header = NULL;
	list->entries = NULL;
	list->next = NULL;

	// last element in list
	struct mekb_ini_list *last = list;

	int res = 1;
	int headers = 0;

	while (1) {
		int eol;
		res = mekb_read_line(read_byte, data, &line_, &eol, alloc);
		// error
		if (res < 0) goto cleanup;
		else
			res = 1;

		// remove comments
		char *comment = strchr(line_, '#');
		if (comment != NULL) *comment = '\0';
		comment = strchr(line_, ';');
		if (comment != NULL) *comment = '\0';

		// trim whitespace
		char *line = mekb_trim_whitespace(line_, '\0');
		if (!line) goto next_line; // empty line

		size_t line_len = strlen(line);

		// check for header brackets
		if (line[0] == '[' && line[line_len - 1] == ']') {
			// remove brackets
			line[line_len - 1] = '\0';
			++line;
			line = mekb_trim_whitespace(line, '\0');
			if (!line) goto cleanup;

			// copy header name for struct
			line = strdup(line);
			if (line == NULL) goto cleanup;

			// create new section
			struct mekb_ini_list *new_list = alloc.malloc(sizeof(struct mekb_ini_list));
			if (new_list == NULL) {
				alloc.free(line);
				goto cleanup;
			}

			new_list->header = line;
			new_list->next = NULL;
			new_list->entries = NULL;

			// append to list
			last->next = new_list;
			last = new_list;
			last_keyval = NULL;

			if (headers < INT_MAX) ++headers;
			goto next_line;
		} else if (line[0] == '[' || line[line_len - 1] == ']') {
			// invalid syntax
			goto cleanup;
		}

		char *eq = strchr(line, '=');
		const char *key, *val;
		if (eq) {
			++eq; // skip =
			key = mekb_trim_whitespace(line, '=');
			val = mekb_trim_whitespace(eq, '\0');
			if (!val) val = "";
		} else {
			key = mekb_trim_whitespace(line, '\0');
			val = NULL;
		}
		if (!key) key = "";

		// duplicate key and value to avoid dangling pointers

		char *key_ = strdup(key);
		if (key_ == NULL) goto cleanup;

		char *val_ = NULL;
		if (val) {
			val_ = strdup(val);
			if (val_ == NULL) {
				alloc.free(key_);
				goto cleanup;
			}
		}

		// create new keyval
		struct mekb_keyval_list *new_keyval = alloc.malloc(sizeof(struct mekb_keyval_list));
		if (new_keyval == NULL) {
			alloc.free(key_);
			if (val_) alloc.free(val_);
			goto cleanup;
		}

		new_keyval->key = key_;
		new_keyval->value = val_;
		new_keyval->next = NULL;

		// append to list
		if (last_keyval)
			last_keyval->next = new_keyval;
		else
			last->entries = new_keyval;
		last_keyval = new_keyval;

	next_line:
		alloc.free(line_);
		line_ = NULL;

		// end of file
		if (eol == EOF) break;
	}

	*out = list;
	return headers;
cleanup:
	alloc.free(line_);
	mekb_free_ini(list, alloc.free);
	return res;
}

void mekb_free_ini_entry(struct mekb_ini_list *list, void (*free)(void *)) {
	if (!list) return;
	for (struct mekb_keyval_list *keyval = list->entries; keyval;) {
		struct mekb_keyval_list *next = keyval->next;
		free(keyval->key);
		free(keyval->value);
		free(keyval);
		keyval = next;
	}
	free(list->header);
	free(list);
}

void mekb_free_ini(struct mekb_ini_list *list, void (*free)(void *)) {
	while (list) {
		struct mekb_ini_list *next = list->next;
		mekb_free_ini_entry(list, free);
		list = next;
	}
}

int mekb_ini_serialize(struct mekb_ini_list *in, char **out, struct mekb_alloc alloc) {
	// calculate size of output
	size_t out_size = 0;
	for (struct mekb_ini_list *list = in; list; list = list->next) {
		if (list->header) {
			out_size += strlen(list->header) + 3; // [header]\n
		}
		for (struct mekb_keyval_list *keyval = list->entries; keyval; keyval = keyval->next) {
			out_size += strlen(keyval->key); // key
			if (keyval->value) {
				out_size += strlen(keyval->value) + 1; // =value
			}
			++out_size; // \n
		}
	}
	if (out_size == 0) {
		*out = NULL;
		return 0;
	}

	++out_size; // null terminator

	// allocate output
	char *out_ = alloc.malloc(out_size);
	if (out_ == NULL) return -1;

	// write to output
	size_t i = 0;
#define MEKB_PRINT(...)                                          \
	{                                                            \
		int ret = snprintf(out_ + i, out_size - i, __VA_ARGS__); \
		if (ret < 0 || (size_t) ret >= out_size - i) goto fail;  \
		i += ret;                                                \
	}
	for (struct mekb_ini_list *list = in; list; list = list->next) {
		if (list->header) {
			MEKB_PRINT("[%s]\n", list->header);
		}
		for (struct mekb_keyval_list *keyval = list->entries; keyval; keyval = keyval->next) {
			MEKB_PRINT("%s", keyval->key);
			if (keyval->value) {
				MEKB_PRINT("=%s", keyval->value);
			}
			MEKB_PRINT("\n");
		}
	}
#undef MEKB_PRINT
	*out = out_;
	if (out_size > INT_MAX) return INT_MAX;
	return out_size - 1;
fail:
	alloc.free(out_);
	return -1;
}
#endif
#endif
