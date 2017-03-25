#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define MAX_DIR_SIZE 256

int print_fileStat(char* inputDir, int simple) {

	DIR* dir;
	struct dirent* dptr;
	struct stat fileStat;
	int ret;

	if (!inputDir) {
		printf("The current directory is NULL\n");
		exit(-1);
	}

	/* get the input directory */
	dir = opendir(inputDir);
	if (!dir) {
		printf("Can not open the input directory\n");
		exit(-1);
	}
	/* print names of files in the directory */
	while ((dptr = readdir(dir)) != NULL) {
		printf(" [%s] ", dptr->d_name);
	}

	printf("\n");

	if (simple == 1)
		return 1;

	dir = opendir(inputDir);
	if (!dir) {
		printf("Can not open the input directory\n");
		exit(-1);
	}

	/* print all the files info in the input directory */
	printf("Printing information about each file in the directory: %s\n",
			inputDir);

	while ((dptr = readdir(dir)) != NULL) {

		ret = stat(dptr->d_name, &fileStat);

		/*	if (ret != 0) {
		 perror("Error stat");
		 exit(2);
		 }*/

		printf("-------------Information about %s------------\n", dptr->d_name);

		printf("File Size: \t\t%6d bytes\n", (int) fileStat.st_size);

		printf("Number of Blocks: \t%6d\n", (int) fileStat.st_blocks);

		printf("Number of Link Count: \t%6d\n", (int) fileStat.st_nlink);

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
		printf("\n RWX for user \t");
		printf(( S_IRUSR & fileStat.st_mode) ? "r" : "-");
		printf(( S_IWUSR & fileStat.st_mode) ? "w" : "-");
		printf(( S_IXUSR & fileStat.st_mode) ? "x" : "-");
		printf("\n RWX for group \t");
		printf(( S_IRGRP & fileStat.st_mode) ? "r" : "-");
		printf(( S_IWGRP & fileStat.st_mode) ? "w" : "-");
		printf(( S_IXGRP & fileStat.st_mode) ? "x" : "-");
		printf("\n RWX for other \t");
		printf(( S_IROTH & fileStat.st_mode) ? "r" : "-");
		printf(( S_IWOTH & fileStat.st_mode) ? "w" : "-");
		printf(( S_IXOTH & fileStat.st_mode) ? "x" : "-");

		printf("\n");

		printf("File Inode: \t\t%d\n", (int) fileStat.st_ino);
		printf("\n");

	}
	return 0;

}

int main(int argc, char *argv[]) {
	int simple = 0;
	/* Interface */

	if (argc == 2 && strcmp(argv[1], "-l")) {
		printf("USAGE: myls \n");
		printf("USAGE: myls -l \n");
		printf("USAGE: myls -l <directory>\n");
		return 1;
	} else if (argc > 3) {
		printf("USAGE: myls \n");
		printf("USAGE: myls -l \n");
		printf("USAGE: myls -l <directory>\n");
		return 1;
	}

	/*buff for the directory */
	char *buff = (char*) malloc(sizeof(char) * MAX_DIR_SIZE);
	if (argc == 1) {
		simple = 1; /* print simple version with only file names */

		/* get current directory */
		buff = getcwd(buff, MAX_DIR_SIZE - 1);
		/* print all files name in the directory */
		print_fileStat(buff, simple);
	}

	if (argc == 2) {
		simple = 0;
		/* Get current directory */
		buff = getcwd(buff, MAX_DIR_SIZE - 1);

		/* print all file info in the directory */
		//printf("Printing information about each file in current directory:\n");
		print_fileStat(buff, simple);

	}

	if (argc == 3) {
		simple = 0;
		/* copy the input directory to buff */
		strcpy(buff, argv[2]);

		/* print the directory contents */
		//printf("Printing information about each file in input directory:\n");
		print_fileStat(buff, simple);

	}

	/* free buff */
	free(buff);

	return 0;
}
