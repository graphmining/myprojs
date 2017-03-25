#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {

	/* Interface */

	if (argc != 2) {
		printf("USAGE: mystat <directory/file>\n");
		return 1;
	}

	struct stat fileStat;

	int ret = stat(argv[1], &fileStat);

	if (ret != 0) {
		perror("Error stat");
		exit(2);
	}

	printf("-------------Information about %s------------\n", argv[1]);

	printf("File Size: \t\t%6d bytes\n", (int) fileStat.st_size);

	printf("Number of Blocks: \t%6d\n", (int) fileStat.st_blocks);

	printf("Number of Link Counts: \t%6d\n", (int) fileStat.st_nlink);

	printf("File Permissions: ");

	printf(
			"(d: Directory. \n r : Read permission. w :Write permission. \n x : Execute permission. - : No permission.)");

	/*
	 * Permissions: read, write, execute, for user, group, others.
	 S_IRWXU S_IRUSR S_IWUSR S_IXUSR
	 S_IRWXG S_IRGRP S_IWGRP S_IXGRP
	 S_IRWXO S_IROTH S_IWOTH S_IXOTH
	 */
	printf("\n directory \t");
	printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
	printf("\n rwx for user \t");
	printf(( S_IRUSR & fileStat.st_mode) ? "r" : "-");
	printf(( S_IWUSR & fileStat.st_mode) ? "w" : "-");
	printf(( S_IXUSR & fileStat.st_mode) ? "x" : "-");
	printf("\n rwx for group \t");
	printf(( S_IRGRP & fileStat.st_mode) ? "r" : "-");
	printf(( S_IWGRP & fileStat.st_mode) ? "w" : "-");
	printf(( S_IXGRP & fileStat.st_mode) ? "x" : "-");
	printf("\n rwx for other \t");
	printf(( S_IROTH & fileStat.st_mode) ? "r" : "-");
	printf(( S_IWOTH & fileStat.st_mode) ? "w" : "-");
	printf(( S_IXOTH & fileStat.st_mode) ? "x" : "-");

	printf("\n");

	printf("File Inode: \t\t%d\n", (int) fileStat.st_ino);
	printf("\n");

	return 0;

}
