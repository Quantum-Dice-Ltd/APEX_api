#include "qdapex_api.h"
#include "util.h"

#define arlen(x) (sizeof(x)/sizeof(*x))

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int gettime(struct timespec *now)
{
 if (clock_gettime(CLOCK_REALTIME, now) == -1) {
	perror("clock_gettime(CLOCK_MONOTONIC) failed");
	fprintf(stderr, "Cannot get time for QDRNG timeouts\n");
	return 1;
 }
 return 0;
}

int main(int argc, char **argv)
{
 (void) argc;
 ++argv;
 const char *dbgfname = 0;
 if (*argv && !strcmp(*argv, "-d") && argv[1]) {
	dbgfname = argv[1];
	argv += 2;
 }
 if (!*argv || argv[1]) {
	fprintf(stderr, "Usage: udptest [-d dbgfile] eth-dev\n");
	return 1;
 }
 QDAPEX qru;
 if (qdapex(&qru, *argv)) {
	fprintf(stderr, "Cannot open qdapex UDP\n");
	return 1;
 }
 if (dbgfname) {
	FILE *dbgf = fopen(dbgfname, "w");
	if (!dbgf) {
		perror("fopen() failed");
		fprintf(stderr, "Cannot create debug file: %s\n", dbgfname);
		return 1;
	}
	setvbuf(dbgf, 0, _IOLBF, 0);
	qdapex_debug(&qru, dbgf);
 }
 int err = 0;
 size_t got = 0;
 size_t lastdot_qty = 0;
 time_t lastdot_time = 0;
 struct timespec start;
 if (gettime(&start)) return 1;
 for (ssize_t limit = 4294967296ull; limit; ) {
	char buf[32768];
	ssize_t rq = limit;
	if (rq + 0ul > sizeof(buf)) rq = sizeof(buf);
	ssize_t nr = qdapex_read(&qru, buf, rq);
	if (nr == -1) {
		fprintf(stderr, "Cannot read from QRNG\n");
		err = 1;
		break;
	} else {
		got += nr;
		limit -= nr;
		ssize_t nw = write(1, buf, nr);
		if (nw == -1) {
			perror("write() failed");
			fprintf(stderr, "Cannot write random data to stdout\n");
			err = 1;
			break;
		}
		if (nw < nr) {
			fprintf(stderr, "Wrote only %ld of %ld bytes of random data to stdout\n", nw, nr);
			err = 1;
			break;
		}
		time_t now = time(0);
		if (now != lastdot_time && lastdot_qty != got) {
			lastdot_time = now;
			lastdot_qty = got;
			putc('.', stderr);
			fflush(stderr);
		}
	}
 }
 struct timespec end;
 if (gettime(&end)) return 1;
 qdapex_dtor(&qru);
 double d = secsince(end, start);
 fprintf(stderr, "\n%f sec", d);
 if (d) d = got / d;
 d *= 8;	// convert to bits/s
 char unit;
 d = to_si(&unit, d);
 fprintf(stderr, "\t%f %cb/s\n", d, unit);
 if (err) fprintf(stderr, "nettest failed\n");
 return err;
}
