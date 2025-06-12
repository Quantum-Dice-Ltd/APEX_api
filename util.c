
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SEC_NSEC (1000*1000*1000)

void ptcheck(int e, const char *msg)
{
	if (!e) return;
	errno = e;
	perror("pthreads function failed");
	fprintf(stderr, "%s failed\n", msg);
	abort();
}

double to_si(char *unitp, double d)
{
	if (d == 0) {
		*unitp = ' ';
		return d;
	}
	int neg = d < 0;
	if (neg) d = -d;
	const char *unitnames = "qryzafpnum kMGTPEZYRQ";
	const char *unit = unitnames + 10;	// starts at the space
	for (;;) {
		if (d >= 1000) {
			if (!*++unit) {	// so we get 12345 Q etc
				unit--;
				break;
			}
			d /= 1000;
		} else if (d < 1) {
			d *= 1000;
			if (--unit == unitnames) break; // se we get 0.1 q etc
		} else break;
	}
	*unitp = *unit;
	if (neg) d = -d;
	return d;
}

double secsince(struct timespec now, struct timespec then)
{
	double dt = now.tv_nsec - then.tv_nsec;
	dt /= SEC_NSEC;
	dt += now.tv_sec - then.tv_sec;
	return dt;
}

