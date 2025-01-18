#ifndef MEKB_RECURSE_ONCE
#define MEKB_RECURSE_ONCE
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

// recurse into a directory
// callback.st: stat of the current path
// callback.depth: depth of the current path
// callback.ddepth: change in depth, 1 for entering a directory, -1 for leaving a directory, 0 for a non-directory
typedef bool (*mekb_recurse_callback)(const char *path, struct stat st, int depth, int ddepth, void *data);
bool mekb_recurse(const char *path, mekb_recurse_callback callback, void *data, void *(*malloc)(size_t), void (*free)(void *));
char *mekb_concat_path(const char *path1, const char *path2, void *(*malloc)(size_t), void (*free)(void *));

#endif
#ifdef MEKB_RECURSE_IMPL
#ifndef MEKB_RECURSE_IMPL_ONCE
#define MEKB_RECURSE_IMPL_ONCE
char *mekb_concat_path(const char *path1, const char *path2, void *(*malloc)(size_t), void (*free)(void *)) {
	size_t new_path_len = strlen(path1) + strlen(path2) + 2;
	char *new_path = malloc(new_path_len);
	if (new_path == NULL) return NULL;

	int r = snprintf(new_path, new_path_len, "%s%c%s", path1,
#ifdef _WIN32
	                 '\\',
#else
	                 '/',
#endif
	                 path2);
	if (r < 0 || (size_t) r >= new_path_len) {
		free(new_path);
		return NULL;
	}

	return new_path;
}
static bool mekb_recurse_internal_(const char *path, mekb_recurse_callback callback, void *data, void *(*malloc)(size_t), void (*free)(void *), int depth) {
	// stat path
	struct stat st;
	if (lstat(path, &st) != 0) return true;

	if (S_ISDIR(st.st_mode)) {
		// read directory
		DIR *dir = opendir(path);
		if (dir == NULL) return true;

		if (!callback(path, st, depth, 1, data)) {
			closedir(dir);
			return false;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.') {
				// skip current and parent dir
				if (entry->d_name[1] == '\0') continue;
				if (entry->d_name[1] == '.' && entry->d_name[2] == '\0') continue;
			}

			// concat paths
			char *entry_path = mekb_concat_path(path, entry->d_name, malloc, free);
			if (entry_path == NULL) {
				closedir(dir);
				return false;
			}

			if (!mekb_recurse_internal_(entry_path, callback, data, malloc, free, depth + 1)) return false;
			free(entry_path);
		}

		closedir(dir);

		if (!callback(path, st, depth, -1, data)) return false;
	} else {
		if (!callback(path, st, depth, 0, data)) return false;
	}

	return true;
}

bool mekb_recurse(const char *path, mekb_recurse_callback callback, void *data, void *(*malloc)(size_t), void (*free)(void *)) {
	return mekb_recurse_internal_(path, callback, data, malloc, free, 0);
}
#endif
#endif
