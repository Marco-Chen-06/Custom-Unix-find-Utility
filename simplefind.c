#include "simplefind.h"

// prints the mode of an inode in human-readable format
int print_mode(mode_t st_mode) {
	char buffer[11];
	// handle leftmost bit
	switch (st_mode & S_IFMT) {
		case S_IFREG: 
			buffer[0] = '-';
		case S_IFDIR:
			buffer[0] = 'd';
		case S_IFLNK:
			buffer[0] = 'l';
		case S_IFCHR:
			buffer[0] = 'c';
		case S_IFBLK:
			buffer[0] = 'b';
		case S_IFSOCK:
			buffer[0] = 's';
		case S_IFIFO:
			buffer[0] = 'p';

	}
}

int print_verbose(char *buffer) {
	struct stat st;
	if (stat(buffer, &st) == -1) {
		fprintf(stderr, "Failed to get stat struct for path: %s for verbose print. %s \n", buffer, strerror(errno));
		return 255;
	}
	// print verbose inode data
	printf("%d     ", st.st_ino);
	printf("%d  ", st.st_blocks / 2); // st_blocks gives units of 512 bytes, so divide by 2 for units of 1k bytes
	print_mode(st.st_mode);
	return 0;
}

int print_info(bool ls_flag, bool xdev_flag, bool name_flag, char *name_pattern, char *starting_path, char *buffer, struct dirent *nextdir) {
	if (name_flag) {
		if (fnmatch(name_pattern, nextdir->d_name, 0) == 0) {
			if (ls_flag) {
				print_verbose(buffer);
			} else {
				printf("%s\n", buffer);
			}
		}
		return 0;
	}
	if (ls_flag) {
		print_verbose(buffer);
	} else {
		printf("%s\n", buffer);
	}
	return 0;
}

int recursive_dfs_search(bool ls_flag, bool xdev_flag, bool name_flag, char *name_pattern, char *starting_path) {
	DIR *currentdir;
	if ((currentdir = opendir(starting_path)) == NULL) {
		fprintf(stderr, "Failed to open path %s: %s\n", starting_path, strerror(errno));
	}

	// get the stat struct for the starting path
	struct stat start_st;
	if (stat(starting_path, &start_st) == -1) {
		fprintf(stderr, "Failed to get stat struct for starting path: %s. %s \n", starting_path, strerror(errno)); 
	}

	if (currentdir == NULL) {
		return -1;
	}

	struct dirent *nextdir;
	char buffer[1024];

	while(true) {
		if (((nextdir = readdir(currentdir)) == NULL) && (errno != 0)) {
			fprintf(stderr, "Failed to read directory %s: %s\n", starting_path, strerror(errno)); 
		}

		if (nextdir == NULL) {
			break;
		}

		snprintf(buffer, 1024, "%s/%s", starting_path, nextdir->d_name);

		// in the case xdev_flag is set, skip directories on filesystems different from starting_path's
		if (xdev_flag) {
			struct stat next_st;
			if (stat(buffer, &next_st) == -1) {
				fprintf(stderr, "Failed to get stat struct for path: %s. %s \n", buffer, strerror(errno));
			}
			if (start_st.st_dev != next_st.st_dev) {
				continue;
			}
		}

		if (strcmp(nextdir->d_name, ".") != 0 && strcmp(nextdir->d_name, "..") != 0) {
			print_info(ls_flag, xdev_flag, name_flag, name_pattern, starting_path, buffer, nextdir);
			// printf("%s\n", buffer);
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

