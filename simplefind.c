#include "simplefind.h"

static int print_path(mode_t st_mode, st_size *buffer) {
}

static int print_formatted_time(time_t mtime) {
	// using example from man 3 strftime
	char outstr[200];
	struct tm *tmp;
	tmp = localtime(&mtime);
	if ((strftime(outstr, sizeof(outstr), "%b %e %H:%M", tmp)) == 0) {
		fprintf(stderr, "Error: Strftime failed to format st_mtime");
		return 255;
	}
	printf("%s", outstr);
	return 0;
}


// Print the size in bytes of the node. For BLK or CHR devices, print the major and minor 
// device numbers. Returns 0 if size is printed, and 1 if major and minot numbers are printed.
static int print_size(mode_t st_mode, off_t st_size, dev_t st_rdev) {
	if (((st_mode & S_IFMT) == S_IFBLK) || ((st_mode & S_IFMT) == S_IFCHR)) {
		printf("%4d ", major(st_rdev));
		printf("%4d ", minor(st_rdev));
		return 1;
	}
	printf("%8d ", st_size);
	return 0;
}

// print the gid or group corresponding to the id. Returns 0 if group is printed, and 1 if gid is printed.
static int print_group(gid_t st_gid) {
	struct group *grp;

	if ((grp = getgrgid(st_gid)) == NULL) {
		printf("%d    ", st_gid);
		return 1;
	}
	printf("%s    ", grp->gr_name);
	return 0;
}

// print the userid or username corresponding to the id. Returns 0 if username is printed, and 1 if uid is printed.
static int print_user(uid_t st_uid) {
	struct passwd *pwd;

	if ((pwd = getpwuid(st_uid)) == NULL) {
		printf("%d    ", st_uid);
		return 1;
	}
	printf("%s    ", pwd->pw_name);
	return 0;
}

// prints the mode of an inode in human-readable format, like verbose ls
static int print_mode(mode_t st_mode) {
	char buffer[11];
	// handle leftmost bit
	switch (st_mode & S_IFMT) {
		case S_IFREG: 
			buffer[0] = '-';
			break;
		case S_IFDIR:
			buffer[0] = 'd';
			break;
		case S_IFLNK:
			buffer[0] = 'l';
			break;
		case S_IFCHR:
			buffer[0] = 'c';
			break;
		case S_IFBLK:
			buffer[0] = 'b';
			break;
		case S_IFSOCK:
			buffer[0] = 's';
			break;
		case S_IFIFO:
			buffer[0] = 'p';
			break;
	}

	// brute force all user group and other permissions before worrying about SUID SGID and sticky bits
	buffer[1] = (st_mode & S_IRUSR) ? 'r' : '-';
	buffer[2] = (st_mode & S_IWUSR) ? 'w' : '-';
	buffer[3] = (st_mode & S_IXUSR) ? 'x' : '-';
	buffer[4] = (st_mode & S_IRGRP) ? 'r' : '-';
	buffer[5] = (st_mode & S_IWGRP) ? 'w' : '-';
	buffer[6] = (st_mode & S_IXGRP) ? 'x' : '-';
	buffer[7] = (st_mode & S_IROTH) ? 'r' : '-';
	buffer[8] = (st_mode & S_IWOTH) ? 'w' : '-';
	buffer[9] = (st_mode & S_IXOTH) ? 'x' : '-';

	buffer[10] = '\0';
        /*
	 * handle SUID SGID and sticky bits. The s and t are capitalized if the
	 * execute permission is not set.
	 */
        if (st_mode & S_ISUID) {
		buffer[3] = (buffer[3] == 'x') ? 's' : 'S';
	}
	if (st_mode & S_ISGID) {
		buffer[6] = (buffer[6] == 'x') ? 's' : 'S';
	}
	if (st_mode & S_ISVTX) {
		buffer[9] = (buffer[9] == 'x') ? 't' : 'T';
	}

	printf("%s   ", buffer);
}

int print_verbose(char *buffer) {
	struct stat st;
	if (stat(buffer, &st) == -1) {
		fprintf(stderr, "Failed to get stat struct for path: %s for verbose print. %s \n", buffer, strerror(errno));
		return 255;
	}
	// print verbose inode data
	printf("  %d      ", st.st_ino);
	printf("%d ", st.st_blocks / 2); // st_blocks gives units of 512 bytes, so divide by 2 for units of 1k bytes
	print_mode(st.st_mode);
	printf("%d ", st.st_nlink);
	print_user(st.st_uid);
	print_group(st.st_gid);
	print_size(st.st_mode, st.st_size, st.st_rdev);
	print_formatted_time(st.st_mtime);
	printf("\n");
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

