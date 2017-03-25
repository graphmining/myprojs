#include <stdint.h> /* uint64_t */
#include <sys/time.h> /* struct timeval */
#include <time.h> /* gettimeofday */
#include <sched.h>

/* assembly code to return cycles */
static inline unsigned long long RDTSC() {
	unsigned int hi, lo;
	__asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

/* Setting processor affinity to cpu 1 */
void Set_Processpor_Affinity() {
	/*cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0, &mask);*/
	unsigned long mask;
	mask = 2; // affinity to cpu 1
	sched_setaffinity(0, sizeof(mask), &mask);
}

/*return struct timeval which is difference of tv1 and tv2 */
struct timeval *TimeDiff(struct timeval *tv1, struct timeval *tv2) {

	static struct timeval tv;

	tv.tv_sec = tv1->tv_sec - tv2->tv_sec;
	tv.tv_usec = tv1->tv_usec - tv2->tv_usec;

	/* take care of subtraction */
	if (tv.tv_usec < 0){

		tv.tv_usec += 1000000;
		tv.tv_sec--;
	}

	return &tv;
}

int main() {

	Set_Processpor_Affinity();

	double gHz; /*estmated CPU frequency */

	struct timeval begintv, endtv;
	unsigned long long begin = 0, end = 0;

	int ret;
	ret = gettimeofday(&begintv, NULL); //get start time

	begin = RDTSC();
	/* run a large number of intentive loops*/
	unsigned long long i;
	for (i = 0; i < 1000000; i++)
		;
	end = RDTSC();

	ret = gettimeofday(&endtv, NULL); //get end time


	struct timeval *tvDiff = TimeDiff(&endtv, &begintv);

	unsigned long long nsecElapsed = tvDiff->tv_sec * 1000000000LL
			+ tvDiff->tv_usec * 1000;

	gHz = (double) (end - begin) / (double) nsecElapsed;

	printf("Measured CPU GHz: %f\n", gHz);/*print estmated CPU frequency */

	return 0;
}
