/* util.h - miscellaneous utilities */

#include <time.h>

/* check pthreads errors - which should never happen, so we abort if we find one
 * msg is an error message to print on error specifying what failed (e.g. "pthread_create()")
 */
void ptcheck(int e, const char *msg);

/* convert a value to SI metric unit prefix. e.g. 5000 -> 5 k
 * the value returned is normalised into [1..1000) or 0 and *unitp is
 * the prefix letter, or ' ' if the value was already in range
 */
double to_si(char *unitp, double d);

/*
 * convery a time difference (now - then) into seconds
 */
double secsince(struct timespec now, struct timespec then);
