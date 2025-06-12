#include "qdapex_api.h"

#include <stdio.h>

int main(int argc, char **argv)
{
 (void) argc;
 ++argv;
 if (!*argv || argv[1]) {
	fprintf(stderr, "Usage: qdapexmini eth-dev\n");
	return 1;
 }
 QDAPEX qru;
 if (qdapex(&qru, *argv)) {
	fprintf(stderr, "Cannot open qdapex UDP\n");
	return 1;
 }
 char buf[1024];
 ssize_t nr = qdapex_read(&qru, buf, sizeof(buf));
 if (nr == -1) {
	fprintf(stderr, "Cannot read from QRNG\n");
	return 1;
 }
 printf("Random bytes:\n");
 for (unsigned u=0; u < nr; u++) printf(" %02x", buf[u] & 0xff);
 printf("\n");
 qdapex_dtor(&qru);
 return 0;
}
