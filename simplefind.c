#include "simplefind.h"

int recursive_dfs_search(bool ls_flag, bool xdev_flag, bool name_flag, char *name_pattern, char *starting_path) {
	DIR *currentdir = opendir(starting_path);

	if (currentdir == NULL) {
		return -1;
	}

	struct dirent *nextdir;
	char buffer[1024];

	while(true) {
		nextdir = readdir(currentdir);

		if (nextdir == NULL) {
			break;
		}

		if (strcmp(nextdir->d_name, ".") != 0 && strcmp(nextdir->d_name, "..") != 0) {
			snprintf(buffer, 1024, "%s/%s", starting_path, nextdir->d_name);
			printf("%s\n\n", buffer);
			if (nextdir->d_type == DT_DIR) {
				recursive_dfs_search(ls_flag, xdev_flag, name_flag, name_pattern, buffer);
			}
		}
	}

	if (closedir(currentdir) == -1) {
		return -1;
	}

	return 0;
}
