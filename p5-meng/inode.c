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
void Set_Processpor_Affinity() {
	unsigned long mask;
	mask = 2; // affinity to cpu 1
	sched_setaffinity(0, sizeof(mask), &mask);
}


#define BLOCK_SIZE 4096

#define MAXSIZE 4096*32


int main() {

	int file = 0;
	int dataSize;
	int i=0;
	unsigned long long begin = 0, end = 0;
	char *buffer = malloc(sizeof(char)*4096);

	Set_Processpor_Affinity();

	file = open("data2.txt", O_WRONLY|O_APPEND);
	printf("file point: %d\n", file);
	while(i++<32){
	  dataSize = i*BLOCK_SIZE;
		begin = RDTSC();
		 write(file, buffer, 4096);
		end = RDTSC();

	 printf("Blocks written: %d\t Cycle Count: %llu\n", dataSize, (end - begin));
	}

	close(file);

	return 0;
}

