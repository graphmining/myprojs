#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>



/* assembly code to return cycles */
static inline unsigned long long RDTSC() {
	unsigned int hi, lo;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

/* Setting processor affinity to cpu 1 */
void Set_Processor_Affinity() {
	unsigned long mask;
	mask = 2; // affinity to cpu 1
	sched_setaffinity(0, sizeof(mask), &mask);
}


#define INCR_SIZE 64

#define MAXSIZE 1024*16


int main() {

	int file = 0;
	int dataSize;
	unsigned long long begin = 0, end = 0;
	char buffer[INCR_SIZE];

	Set_Processor_Affinity();

	file = open("data.txt", O_RDONLY);
	posix_fadvise(file, 0, 0, POSIX_FADV_RANDOM); /*No sequential reads*/
	posix_fadvise(file, 0, 0, POSIX_FADV_DONTNEED); /*No prefectch*/

	lseek(file, 0, SEEK_SET);/* move file pointer to the beginning */

	for (dataSize = INCR_SIZE; dataSize < MAXSIZE; dataSize = dataSize + INCR_SIZE) {

		begin = RDTSC();
		read(file, buffer, INCR_SIZE);
		end = RDTSC();

		printf("%d\t%llu\n", dataSize, (end - begin));
	}

	close(file);

	return 0;
}
