#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#define INCR_SIZE 1024

#define MAXSIZE 32*1024*32

/* assembly code to return cycles */
static inline unsigned long long RDTSC() {
	unsigned int hi, lo;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

/* Setting processor affinity to cpu 1 */
void Set_Processpor_Affinity() {
	unsigned long mask;
	mask = 2; // affinity to cpu 1
	sched_setaffinity(0, sizeof(mask), &mask);
}

int main() {

	int file = 0;
	int dataSize;
	unsigned long long begin = 0, end = 0;
	char buffer[INCR_SIZE];

	Set_Processpor_Affinity();

	file = open("data.txt", O_RDONLY);

	lseek(file, 0, SEEK_SET);/* move file pointer to the beginning */
	fsync(file);

	for (dataSize = INCR_SIZE; dataSize < MAXSIZE; dataSize = dataSize + INCR_SIZE) {

		begin = RDTSC();
		read(file, buffer, INCR_SIZE);
		end = RDTSC();

		printf("Bytes read: %d\tCycle count: %llu\n", dataSize, (end - begin));
	}

	close(file);

	return 0;
}

