#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

int dir_walk(const char *fileName, const struct stat *fileStat, int type) {

	/* This function is invoked for each object in the
	 directory hierarchy and is passed to ftw by main. */

	if (type == FTW_NS) {
		printf("Error: This file exists, but stat fails for it!\n");
		return 0;
	}
	if (type == FTW_DNR) {

		printf("Error: This is a directory which cannot be read. \n");
		return 0;
	}

	printf("%s\n", fileName);

	return 0;
}
/*  This is information about ftw function
 *   #include <ftw.h>

 int ftw(
 const char *path,
 int(*function)(const char *, const struct stat *,
 int),
 int ndirs);


 PARAMETERS


 path	    Specifies the directory hierarchy to be searched.

 function  Specifies the function to be invoked for each object in the
 directory hierarchy.

 ndirs	    Specifies the maximum number of directory streams or file
 descriptors (or both) available for use by ftw().  This parameter
 is not used in Tru64 UNIX implementations of ftw().

 This parameter must be in the range of 1 to OPEN_MAX.

 For backward compatibility with operating system versions prior
 to DIGITAL UNIX Version 4.O, ftw() takes a depth argument instead
 of ndirs.  The depth parameter specifies the directory depth for
 the search, but it is not used.
 */

int main(int argc, char *argv[]) {

	int ret;

	if (argc == 1) {
		/* use current directory, ndir = 1 means using at most 1 file
		 descriptor */
		ret = ftw(".", dir_walk, 1);
		/* error message */
		if (ret != 0) {
			perror("Error ftw");
			exit(2);
		}
	}

	else if (argc == 2) {
		/* use input directory, ndir = 1 means using at most 1 file
		 descriptor */
		ret = ftw(argv[1], dir_walk, 1);
		/* error message */
		if (ret != 0) {
			perror("Error ftw");
			exit(2);
		}
	} else
		printf("Usage: mysearch <directory>\n");

	return 0;
}
