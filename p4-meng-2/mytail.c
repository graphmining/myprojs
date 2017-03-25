#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {

	if ((argc != 3) || (argv[1][0] != '-')) {
		printf("Usage: mytail -n <file>\n");
		return 1;
	}

	int dir;

	off_t flength, position, reverse;

	int lines;

	if ((argc == 3) && (argv[1][0] == '-')) {
		lines = atoi(strtok(argv[1], "-"));
	}

	/* open file for read */
	dir = open(argv[2], O_RDONLY);

	if (!dir) {
		printf("Can not open the input file directory\n");
		exit(-1);
	}

	flength = lseek(dir, 0, SEEK_END);
	//printf("file length: %d \n", (int) flength);
	if (flength == (off_t) -1) {
		perror("Error lseek");
		exit(1);
	}

	/* malloc memory for buff according to the file size!!!!*/
	char *buf = (char*) malloc(sizeof(char) * (flength + 10));
	if (buf == NULL) {
		fputs("Memory error", stderr);
		exit(2);
	}

	/* from the end of file, count newline */
	int newlines = 0;
	/*reverse postion*/
	reverse = 0;
	/* count newlines from the end */
	while (newlines <= lines && reverse < flength) {
		/* reverse from end one char by one char */
		reverse++;
		position = lseek(dir, -reverse, SEEK_END);
		char ch[1];
		/*read one char each time to buff */
		read(dir, ch, 1);
		/* check if char read is newline */
		if (ch[0] == '\n')
			newlines++;

	}

	if (reverse < flength) {
		lseek(dir, position + 1, SEEK_SET);
		printf("The last %d lines are:\n", lines);
	} else {
		lseek(dir, position, SEEK_SET);
		printf("Warning: You want the last %d lines, this file has %d lines:\n", lines,
				newlines);
	}
	read(dir, buf, (int) (reverse));
	write(1, buf, (int) (reverse));
	fflush(stdout);

	close(dir);
	free(buf);

	return 0;
}
