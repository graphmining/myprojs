#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

#define INCR_SIZE 4096

#define MAXSIZE 1024*1024*4

/* assembly code to read the TSC */
static inline uint64_t RDTSC() {
	unsigned int hi, lo;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t) hi << 32) | lo;
}

/* binding process to cpu 1 */
void InitRdtsc() {
	unsigned long cpuMask;
	cpuMask = 2; // binding process to cpu 1
	sched_setaffinity(0, sizeof(cpuMask), &cpuMask);
}

int main() {
	int file = 0;
	int i, j;
	uint64_t begin = 0, end = 0;
	char buffer[INCR_SIZE];

	InitRdtsc();

	file = open("data.txt", O_RDONLY);

	lseek(file, 0, SEEK_SET);

	for (i = INCR_SIZE; i < MAXSIZE; i = i * 2) {

		begin = RDTSC();
		usleep(1000);
		read(file, buffer, INCR_SIZE);
		end = RDTSC();

		printf("Block: %d\t Cycle Count:%llu\n", i/4096,(unsigned long long)(end - begin));
	}

	close(file);

	return 0;
}

